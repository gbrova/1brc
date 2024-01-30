#include <iostream>
#include <vector>
#include <string>

#include <sstream>
#include <string>
#include <fstream>
#include <map>

#include <thread>
#include <future>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <chrono>

#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

class Summary
{
public:
    int count;
    int sum;
    int min;
    int max;
};

const int CITY_LENGTH = 32;

class City
{

public:
    int size;
    char city[CITY_LENGTH];

    City(char city_c[])
    {
        memcpy(city, city_c, CITY_LENGTH);
    }

    bool operator==(const City &other) const
    {
        return 0 == memcmp(city, other.city, CITY_LENGTH);
    }
};

namespace std
{
    template <>
    struct hash<City>
    {
        size_t operator()(const City &city) const
        {
            // Note: I tried several different hash functions, doesn't seem to make a big difference (except for the first, which is bad).

            // Using this hash function takes ~300ms total.
            // return city.city[0] << 24 + city.city[1] << 16 + city.city[2] << 8 + city.city[3];

            // Using this hash function takes ~32ms total.  (note the parens!)
            return (city.city[0] << 24) + (city.city[1] << 16) + (city.city[2] << 8) + city.city[3];

            // Using this hash function takes ~33ms total. (different primes)
            // return city.city[0] * 977 * 1129 * 983 + city.city[1] * 727 * 1129 + city.city[2] * 727 + city.city[3];

            // Using this hash function takes ~33ms total. (see justification at http://stackoverflow.com/a/1646913/126995)
            // int res = 17;
            // res = res * 31 + city.city[0];
            // res = res * 31 + city.city[1];
            // res = res * 31 + city.city[2];
            // res = res * 31 + city.city[3];
            // return res;
        }
    };
}

void aggregate(std::unordered_map<City, Summary> &temps)
{
    // For simplicity/laziness, we're not sorting the result. But the time taken to sort ~400 rows is trivial, so ignore it for now.
    for (const auto &[city, thisSummary] : temps)
    {
        float min = (float)thisSummary.min / 10;
        float max = (float)thisSummary.max / 10;
        float avg = (float)thisSummary.sum / thisSummary.count / 10;

        fmt::print("{}={:.1f}/{:.1f}/{:.1f}\n", city.city, min, max, avg);
    }
}

void update(Summary &thisSummary, float tempf)
{
    thisSummary.sum += tempf;
    thisSummary.count += 1;

    if (tempf < thisSummary.min)
    {
        thisSummary.min = tempf;
    }

    if (tempf > thisSummary.max)
    {
        thisSummary.max = tempf;
    }
}

int read_city(char city[], char *buffer, int start_pos)
{
    memset(city, 0, CITY_LENGTH);
    for (int i = start_pos;; i++)
    {
        char c = buffer[i];
        if (c == ';')
        {
            city[i - start_pos] = '\0';
            return i + 1;
        }
        city[i - start_pos] = c;
        // city[i - start_pos] = c;
    }

    // should not happen.
    return -1;
}

std::tuple<int, int> read_number(char *buffer, int start_pos)
{
    int tempf = 0;
    int sign = 1;
    for (int i = start_pos;; i++)
    {
        char c = buffer[i];
        if (c == '.')
        {
            continue;
        }
        if (c == '-')
        {
            sign = -1;
            continue;
        }
        if (c == '\n')
        {
            tempf = tempf * sign;
            return {i, tempf};
        }
        int d = c - '0';
        tempf = tempf * 10 + d;
    }

    // should not happen.
    return {-1, -1};
}

void save_temperature(char city[], int tempf, std::unordered_map<City, Summary> &temps)
{
    // TODO: change this std::string into a char*, maybe using ideas from https://stackoverflow.com/questions/20649864/c-unordered-map-with-char-as-key.
    auto city_cls = City(city);
    auto tuple = temps.find(city_cls);

    if (tuple == temps.end())
    {
        // not found
        Summary thisSummary;
        thisSummary.count = 1;
        thisSummary.min = tempf;
        thisSummary.max = tempf;
        thisSummary.sum = tempf;
        temps.insert(std::pair(city_cls, thisSummary));
    }
    else
    {
        Summary &thisSummary = tuple->second;
        update(thisSummary, tempf);
    }
}

std::tuple<char *, std::size_t> mmap_file(char *filepath)
{
    int fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        printf("\n\"%s \" could not open\n", filepath);
        exit(1);
    }
    struct stat statbuf;
    int err = fstat(fd, &statbuf);
    if (err < 0)
    {
        printf("\n\"%s \" could not open\n", filepath);
        exit(2);
    }

    char *buffer = (char *)mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED)
    {
        printf("Mapping Failed\n");
        exit(2);
    }

    return {buffer, statbuf.st_size};
}

std::unordered_map<City, Summary> parse_section(char *buffer, std::size_t start, std::size_t end, std::size_t overshoot)
{
    char city[CITY_LENGTH];
    std::unordered_map<City, Summary> temps;

    // If we're in the middle of a line, advance to the next full line.
    if (start != 0 && buffer[start - 1] != '\n')
    {
        while (true)
        {
            if (buffer[start] == '\n')
            {
                start++;
                break;
            }
            start++;
        }
    }

    for (std::size_t i = start; i < end; i++)
    {
        i = read_city(city, buffer, i);
        auto [ii, tempf] = read_number(buffer, i);
        i = ii;

        save_temperature(city, tempf, temps);
    }

    return temps;
}

std::unordered_map<City, Summary> combine_results(std::unordered_map<City, Summary> results[], int n_threads)
{
    std::unordered_map<City, Summary> combined;
    for (int i = 0; i < n_threads; i++)
    {
        for (const auto &[city, this_summary] : results[i])
        {
            auto tuple = combined.find(city);

            if (tuple == combined.end())
            {
                combined.insert(std::pair(city, this_summary));
            }
            else
            {
                Summary &combined_summary = tuple->second;
                combined_summary.max = max(combined_summary.max, this_summary.max);
                combined_summary.min = min(combined_summary.min, this_summary.min);
                combined_summary.count = combined_summary.count + this_summary.count;
                combined_summary.sum = combined_summary.sum + this_summary.sum;
            }
        }
    }
    return combined;
}

void do_the_work(char *filepath)
{
    auto [buffer, size] = mmap_file(filepath);

    const int n_threads = 4;
    std::future<std::unordered_map<City, Summary>> promises[n_threads];
    std::unordered_map<City, Summary> results[n_threads];

    int stepsize = size / n_threads;
    for (int i = 0; i < n_threads; i++)
    {
        std::size_t overshoot = 100;
        if (i == n_threads - 1)
        {
            overshoot = 0;
        }
        promises[i] = std::async(std::launch::async, parse_section, buffer, i * stepsize, (i + 1) * stepsize, overshoot);
    }

    // aggregate
    for (int i = 0; i < n_threads; i++)
    {
        results[i] = promises[i].get();
    }

    auto combined_results = combine_results(results, n_threads);
    aggregate(combined_results);
}

int main()
{
    auto start = chrono::high_resolution_clock::now();

    do_the_work("measurements.txt.1m");

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "time elapsed (us): " << duration.count() << endl;
}
