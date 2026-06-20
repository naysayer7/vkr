#pragma once
#include <filesystem>
#include <thread>

#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"
#include "utils.hpp"

namespace Controllers {

void TestMemoryDatasetsThreadTarget(AppState& state);

inline void SelectTestMemoryDatasetsFiles(TestMemoryDatasetsSetupState& setup) {
  const char* result =
      tinyfd_openFileDialog("Выберите NPY файлы", "", 0, nullptr, nullptr, 1);
  if (!result || result[0] == '\0')
    return;
  std::string s(result);
  std::size_t pos = 0;
  while (true) {
    auto sep = s.find('|', pos);
    std::string path =
        s.substr(pos, sep == std::string::npos ? std::string::npos : sep - pos);
    if (!path.empty())
      setup.selectedFiles.push_back(std::move(path));
    if (sep == std::string::npos)
      break;
    pos = sep + 1;
  }
}

inline void StartTestMemoryDatasets() {
  AppState& state = AppState::instance();
  state.m_TestMemoryDatasetsState.phase.store(
      TestMemoryDatasetsPhase::Progress);
  std::thread t(TestMemoryDatasetsThreadTarget, std::ref(state));
  t.detach();
}

inline void TestMemoryDatasetsThreadTarget(AppState& state) {
  try {
    TestMemoryDatasetsProgressState& progress =
        state.m_TestMemoryDatasetsState.progress;
    TestMemoryDatasetsResultState& result =
        state.m_TestMemoryDatasetsState.result;
    const TestMemoryDatasetsSetupState& setup =
        state.m_TestMemoryDatasetsState.setup;

    result.Reset();
    progress.Reset();

    const int M = setup.maxEntries;
    // STR (bulk-load) не использует нижнюю границу m; берём валидное (M + 1) / 2.
    const int m = (M + 1) / 2;
    const int measurements = setup.CalculateMeasurements();

    if (measurements <= 0)
      throw std::runtime_error("Не выбрано ни одного файла для тестирования.");

    progress.total = measurements;

    for (int i = 0; i < measurements; ++i) {
      const std::string& filePath = setup.selectedFiles[i];
      const std::string datasetName =
          std::filesystem::path(filePath).stem().string();
      progress.SetCurrentDataset(datasetName);

      npy::npy_data<double> data = Utils::ReadNpyAsDouble(filePath);
      if (data.shape.size() != 2 || data.shape[1] % 2 != 0)
        throw std::runtime_error(
            "Неверный формат файла «" +
            std::filesystem::path(filePath).filename().string() +
            "»: ожидается 2D массив с чётным числом столбцов.");

      const std::size_t n = data.shape[0];
      const std::size_t dims = data.shape[1] / 2;

      std::vector<rtree::Object<double>> objects;
      objects.reserve(n);
      for (std::size_t j = 0; j < n; ++j) {
        rtree::Rectangle<double> rect(dims);
        for (std::size_t d = 0; d < data.shape[1]; ++d)
          rect.size[d] = data.data[j * data.shape[1] + d];
        objects.emplace_back(static_cast<uint64_t>(j), rect);
      }

      // Память зависит только от числа проиндексированных объектов и M, поэтому
      // индексируем весь набор целиком (без hold-out).
      auto tree = std::make_unique<rtree::RTree<double>>(M, m, dims);
      std::vector<const rtree::Object<double>*> objectPtrs;
      objectPtrs.reserve(objects.size());
      for (const auto& obj : objects)
        objectPtrs.push_back(&obj);
      tree->BulkLoad(std::move(objectPtrs));

      const std::size_t mem = tree->MemorySize();
      result.entries.push_back({datasetName, n, mem});

      progress.done++;
    }

    // Сохранение результатов в results/mem_datasets/{time}_M={M}.npy
    // Формат: 2D массив shape (datasets, 2), столбцы: [N, memory_bytes]
    const std::string resultsDir =
        std::filesystem::current_path().string() + "/results/mem_datasets";
    std::filesystem::create_directories(resultsDir);

    const std::string filename = resultsDir + "/" +
                                 std::to_string(std::time(nullptr)) +
                                 "_M=" + std::to_string(M) + ".npy";
    result.savedFilename = filename;

    const std::size_t rows = result.entries.size();
    std::vector<size_t> flat;
    flat.reserve(rows * 2);
    for (const auto& entry : result.entries) {
      flat.push_back(static_cast<size_t>(entry.indexedCount));
      flat.push_back(static_cast<size_t>(entry.memory));
    }

    npy::npy_data_ptr<size_t> out{
        flat.data(), {(npy::ndarray_len_t)rows, 2}, false};
    std::printf("Saving memory-datasets test results to %s\n",
                filename.c_str());
    npy::write_npy(filename, out);

    state.m_TestMemoryDatasetsState.phase.store(
        TestMemoryDatasetsPhase::Results);
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestMemoryDatasetsState.phase.store(TestMemoryDatasetsPhase::Setup);
  }
}

}  // namespace Controllers
