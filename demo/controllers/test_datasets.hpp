#pragma once
#include <filesystem>
#include <thread>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"

namespace Controllers {

void TestDatasetsThreadTarget(AppState& state);

inline void SelectTestDatasetsFiles(TestDatasetsSetupState& setup) {
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

inline void StartTestDatasets() {
  AppState& state = AppState::instance();
  state.m_TestDatasetsState.phase.store(TestDatasetsPhase::Progress);
  std::thread t(TestDatasetsThreadTarget, std::ref(state));
  t.detach();
}

inline void TestDatasetsThreadTarget(AppState& state) {
  try {
    TestDatasetsProgressState& progress = state.m_TestDatasetsState.progress;
    const TestDatasetsSetupState& setup = state.m_TestDatasetsState.setup;

    progress.Reset();

    const int M = setup.maxEntries;
    const int m = setup.minEntries;
    const int k = setup.k;
    const int measurements = setup.CalculateMeasurements();

    if (measurements <= 0)
      throw std::runtime_error("Не выбрано ни одного файла для тестирования.");

    progress.total = measurements;
    progress.epochs = setup.epochs;

    const std::string resultsDir = std::filesystem::current_path().string() +
                                   "/results/datasets/" +
                                   std::to_string(std::time(nullptr));
    std::filesystem::create_directories(resultsDir);

    for (int i = 0; i < measurements; ++i) {
      const std::string& filePath = setup.selectedFiles[i];
      const std::string datasetName =
          std::filesystem::path(filePath).stem().string();
      progress.SetCurrentDataset(datasetName);

      npy::npy_data<float> data = npy::read_npy<float>(filePath);
      if (data.shape.size() != 2 || data.shape[1] % 2 != 0)
        throw std::runtime_error(
            "Неверный формат файла «" +
            std::filesystem::path(filePath).filename().string() +
            "»: ожидается 2D массив с чётным числом столбцов.");

      const std::size_t n = data.shape[0];
      const std::size_t dims = data.shape[1] / 2;

      std::vector<rtree::Object<float>> objects;
      objects.reserve(n);
      for (std::size_t j = 0; j < n; ++j) {
        rtree::Rectangle<float> rect(dims);
        for (std::size_t d = 0; d < data.shape[1]; ++d)
          rect.size[d] = data.data[j * data.shape[1] + d];
        objects.emplace_back(static_cast<uint64_t>(j), rect);
      }

      progress.epochsDone = 0;

      auto tree = std::make_unique<rtree::RTree<float>>(M, m, dims);
      std::vector<const rtree::Object<float>*> objectPtrs;
      objectPtrs.reserve(objects.size());
      for (const auto& obj : objects)
        objectPtrs.push_back(&obj);
      tree->BulkLoad(std::move(objectPtrs));

      std::vector<double> times;
      times.reserve(setup.epochs);
      for (int e = 0; e < setup.epochs; ++e) {
        auto elapsed = Measures::RunMeasure([&]() {
          for (const auto& obj : objects)
            tree->kNN(obj.mbr, k);
        });
        times.push_back(elapsed.count());
        progress.epochsDone++;
      }

      const std::string filename =
          resultsDir + "/" + datasetName + "_results.npy";
      npy::npy_data_ptr<double> out{
          times.data(), {(npy::ndarray_len_t)times.size()}, false};
      std::printf("Saving %s\n", filename.c_str());
      npy::write_npy(filename, out);

      progress.done++;
    }

    state.m_TestDatasetsState.phase.store(TestDatasetsPhase::Results);
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestDatasetsState.phase.store(TestDatasetsPhase::Setup);
  }
}

}  // namespace Controllers
