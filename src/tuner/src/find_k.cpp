#include <find_k.hpp>

#include <algorithm>
#include <cmath>
#include <thread>

namespace tuner {

double expectedResult(double eval, double k) {
    // 1 / (1 + 10^(-k * eval / 400)): 0.5 at eval 0, towards 1 as white's eval grows, towards 0 as it falls.
    return 1.0 / (1.0 + std::pow(10.0, -k * eval / 400.0));
}

double meanSquaredError(const TunerData& data, double k, std::size_t threads) {
    const std::size_t count = data.entries.size();
    if (count == 0)
        return 0.0;
    threads = std::clamp<std::size_t>(threads, 1, count);

    // each thread sums the squared errors of its own slice into its own slot, nothing is shared while they run.
    std::vector<double> sums(threads, 0.0);
    {
        std::vector<std::jthread> workers;
        workers.reserve(threads);
        for (std::size_t t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] {
                const std::size_t begin = count * t / threads;
                const std::size_t end = count * (t + 1) / threads;
                double sum = 0.0;
                for (std::size_t i = begin; i < end; ++i) {
                    const TunerEntry& entry = data.entries[i];
                    // how far the evaluation's prediction was from what happened in the game.
                    const double miss = entry.result - expectedResult(entry.staticEval, k);
                    sum += miss * miss;
                }
                sums[t] = sum;
            });
        }
    }   // joins the workers

    // added in slice order, so the total is the same for any number of threads.
    double total = 0.0;
    for (double sum : sums)
        total += sum;
    return total / static_cast<double>(count);
}

KSearch findK(const TunerData& data, std::size_t threads) {
    KSearch search;

    // the window of K to look in and the step to look in it with, both shrink by 10 each round.
    double low = 0.0;
    double high = 3.0;
    double step = 0.1;
    double bestK = 0.0;
    double bestError = 1.0;   // larger than any error, (result - expected)^2 is at most 1

    for (int round = 0; round < 4; ++round) {
        // k from the step count rather than adding step up, repeated additions of 0.1 drift.
        for (int i = 0; low + i * step <= high + step / 2; ++i) {
            const double k = low + i * step;
            const double error = meanSquaredError(data, k, threads);
            search.tried.emplace_back(k, error);
            if (error < bestError) {
                bestError = error;
                bestK = k;
            }
        }
        // the lowest error is between the two Ks next to the best one, look there with a 10 times finer step.
        low = std::max(0.0, bestK - step);
        high = bestK + step;
        step /= 10.0;
    }

    search.k = bestK;
    search.error = bestError;
    return search;
}

} // namespace tuner
