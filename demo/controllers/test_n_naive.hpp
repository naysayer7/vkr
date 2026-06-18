#pragma once
#include <algorithm>
#include <filesystem>
#include <thread>

#include "../measures.hpp"
#include "error.hpp"
#include "npy.hpp"
#include "rtree.h"
#include "state.hpp"

namespace Controllers {

void TestNNaiveThreadTarget(AppState& state);

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

inline void SelectTestNNaiveFiles(TestNNaiveSetupState& setup) {
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

inline void StartTestNNaive() {
  AppState& state = AppState::instance();
  state.m_TestNNaiveState.phase.store(TestNNaivePhase::Progress);
  std::thread t(TestNNaiveThreadTarget, std::ref(state));
  t.detach();
}

inline void TestNNaiveThreadTarget(AppState& state) {
  try {
    TestNNaiveProgressState& progress = state.m_TestNNaiveState.progress;
    const TestNNaiveSetupState& setup = state.m_TestNNaiveState.setup;

    progress.Reset();

    const int k = setup.k;
    const int measurements = setup.CalculateMeasurements();

    if (measurements <= 0)
      throw std::runtime_error("Не выбрано ни одного файла для тестирования.");

    progress.total = measurements;
    progress.epochs = setup.epochs;

    const std::string resultsDir = std::filesystem::current_path().string() +
                                   "/results/n_naive/" +
                                   std::to_string(std::time(nullptr));
    std::filesystem::create_directories(resultsDir);

    for (int i = 0; i < measurements; ++i) {
      const std::string& filePath = setup.selectedFiles[i];

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

      progress.currentN = static_cast<int>(n);
      progress.epochsDone = 0;

      std::vector<double> times;
      times.reserve(setup.epochs);
      for (int e = 0; e < setup.epochs; ++e) {
        auto elapsed = Measures::RunMeasure([&]() {
          for (const auto& obj : objects)
            NaiveKnn(objects, obj.mbr, k);
        });
        times.push_back(elapsed.count());
        progress.epochsDone++;
      }

      const std::string filename = resultsDir + "/" + std::to_string(n) + "_" +
                                   std::to_string(k) + ".npy";
      npy::npy_data_ptr<double> out{
          times.data(), {(npy::ndarray_len_t)times.size()}, false};
      std::printf("Saving %s\n", filename.c_str());
      npy::write_npy(filename, out);

      progress.done++;
    }

    state.m_TestNNaiveState.phase.store(TestNNaivePhase::Results);
  } catch (const std::exception& e) {
    Error::Handle(e);
    state.m_TestNNaiveState.phase.store(TestNNaivePhase::Setup);
  }
}

}  // namespace Controllers
