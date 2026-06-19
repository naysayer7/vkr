#pragma once
#include <algorithm>
#include <filesystem>
#include <thread>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"
#include "utils.hpp"

namespace Controllers {

void TestDatasetsNaiveThreadTarget(AppState& state);

// Наивный kNN: перебор всех объектов с вычислением расстояния до запроса и
// выбором k ближайших. Расстояния до всех объектов считаются за O(N), отбор k
// ближайших — за O(N + k·log k): nth_element даёт линейный (в среднем) разбор по
// k-му элементу, затем сортируются только эти k штук для порядка по возрастанию
// расстояния (как у RTree::kNN). Используется как эталон для сравнения.
template <typename T>
inline std::vector<const rtree::Object<T>*> NaiveKnn(
    const std::vector<rtree::Object<T>>& objects,
    const rtree::Rectangle<T>& query, std::size_t k) {
  std::vector<std::pair<T, const rtree::Object<T>*>> dists;
  dists.reserve(objects.size());
  for (const auto& obj : objects)
    dists.emplace_back(obj.mbr.MinDistanceSq(query), &obj);

  const std::size_t kk = std::min(k, dists.size());
  const auto byDist = [](const auto& a, const auto& b) {
    return a.first < b.first;
  };
  if (kk < dists.size())
    std::nth_element(dists.begin(), dists.begin() + kk, dists.end(), byDist);
  std::sort(dists.begin(), dists.begin() + kk, byDist);

  std::vector<const rtree::Object<T>*> result;
  result.reserve(kk);
  for (std::size_t i = 0; i < kk; ++i)
    result.push_back(dists[i].second);
  return result;
}

inline void SelectTestDatasetsNaiveFiles(TestDatasetsNaiveSetupState& setup) {
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

inline void StartTestDatasetsNaive() {
  AppState& state = AppState::instance();
  state.m_TestDatasetsNaiveState.phase.store(TestDatasetsNaivePhase::Progress);
  std::thread t(TestDatasetsNaiveThreadTarget, std::ref(state));
  t.detach();
}

inline void TestDatasetsNaiveThreadTarget(AppState& state) {
  try {
    TestDatasetsNaiveProgressState& progress = state.m_TestDatasetsNaiveState.progress;
    const TestDatasetsNaiveSetupState& setup = state.m_TestDatasetsNaiveState.setup;

    progress.Reset();

    const int k = setup.k;
    const int queryCount = setup.queryCount;
    const int measurements = setup.CalculateMeasurements();

    if (measurements <= 0)
      throw std::runtime_error("Не выбрано ни одного файла для тестирования.");

    progress.total = measurements;
    progress.epochs = setup.epochs;

    const std::string resultsDir = std::filesystem::current_path().string() +
                                   "/results/datasets_naive/" +
                                   std::to_string(std::time(nullptr));
    std::filesystem::create_directories(resultsDir);

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

      progress.epochsDone = 0;

      const Utils::Bounds bounds = Utils::ComputeBounds(data);
      const auto queries = Utils::GenerateUniformQueries(
          bounds, static_cast<std::size_t>(queryCount), Utils::kQuerySeed);

      std::vector<double> times;
      times.reserve(setup.epochs);
      for (int e = 0; e < setup.epochs; ++e) {
        auto elapsed = Measures::RunMeasure([&]() {
          for (const auto& q : queries)
            NaiveKnn(objects, q, k);
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

    state.m_TestDatasetsNaiveState.phase.store(TestDatasetsNaivePhase::Results);
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestDatasetsNaiveState.phase.store(TestDatasetsNaivePhase::Setup);
  }
}

}  // namespace Controllers
