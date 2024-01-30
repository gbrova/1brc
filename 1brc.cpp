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

#include <sys/mman.h>

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

// Try to write a custom hash function.
class City
{

public:
    int size;
    char city[CITY_LENGTH];

    City(char city_c[])
    {
        memset(city, 0, CITY_LENGTH);
        for (int i = 0; i < CITY_LENGTH; i++)
        {
            char c = city_c[i];
            city[i] = c;
            if (c == '\0')
            {
                size = i;
                break;
            }
        }
        // memcpy(city, city_c, CITY_LENGTH);
    }

    bool operator==(const City &other) const
    {
        // return (city == other.city);
        int rval = memcmp(city, other.city, CITY_LENGTH);
        // cout << "rval: " << city << " vs " << other.city << " was " << rval << endl;
        return rval == 0;
    }
};
namespace std
{
    template <>
    struct hash<City>
    {
        size_t operator()(const City &city) const
        {
            // This approach turns out to be pretty bad..
            // return city.city[0] << 24 + city.city[1] << 16 + city.city[2] << 8 + city.city[3];

            // Instead, this seems to work pretty well/fast (inspired by https://stackoverflow.com/a/17017281)
            int res = 17;
            res = res * 31 + city.city[0];
            res = res * 31 + city.city[1];
            res = res * 31 + city.city[2];
            res = res * 31 + city.city[8];
            return res;
        }
    };
}

// kcachegrind callgrind.out.11921 and before that, valgrind --tool=callgrind ./helloworld
// or try gprof

void aggregate(std::unordered_map<City, Summary> &temps)
{
    // TODO we probably have to sort first!
    // for (std::unordered_map<std::string, Summary>::iterator iter = temps.begin(); iter != temps.end(); ++iter)
    for (const auto &[city, thisSummary] : temps)
    {
        // std::string city = iter->first;
        // Summary thisSummary = iter->second;

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
    for (int i = start_pos; i < start_pos + 100; i++)
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
    for (int i = start_pos; i < start_pos + 100; i++)
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

void remember(char city[], int tempf, std::unordered_map<City, Summary> &temps)
{
    City city_class = City(city);

    auto tuple = temps.find(city_class);

    if (tuple == temps.end())
    {
        // cout << "new city: " << city << endl;
        // not found
        Summary thisSummary;
        thisSummary.count = 1;
        thisSummary.min = tempf;
        thisSummary.max = tempf;
        thisSummary.sum = tempf;

        temps.insert(std::pair(city_class, thisSummary));
    }
    else
    {
        // cout << "old city" << city << endl;
        // TODO understand why new cities do not hit the other branch. What's going on!!

        Summary &thisSummary = tuple->second;
        update(thisSummary, tempf);
    }
}

std::unordered_map<City, Summary> read_file(char *buffer, std::streamsize start, std::streamsize end, std::streamsize extra)
{
    int last = 0;
    char city[CITY_LENGTH];
    std::unordered_map<City, Summary> temps;

    for (int i = start; i < end; i++)
    {
        i = read_city(city, buffer, i);

        auto [ii, tempf] = read_number(buffer, i);
        i = ii;

        remember(city, tempf, temps);
    }

    return temps;
}

std::tuple<char *, int> mmap_file()
{
    // TODO: where I am right now: 10m runs in 300ms on 1 thread.
    char *filepath = "measurements.txt.1m";
    int fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        printf("\n\"%s \" could not open\n",
               filepath);
        exit(1);
    }
    struct stat statbuf;
    int err = fstat(fd, &statbuf);
    if (err < 0)
    {
        printf("\n\"%s \" could not open\n",
               filepath);
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

void combine_results(std::unordered_map<City, Summary> results[], int n_results, std::unordered_map<City, Summary> &combined_results)
{
    for (int i = 0; i < n_results; i++)
    {
        auto result = results[i];
        for (const auto &[city, this_summary] : result)
        {
            auto tuple = combined_results.find(city);
            if (tuple == combined_results.end())
            {
                Summary new_summary;
                new_summary.count = this_summary.count;
                new_summary.min = this_summary.min;
                new_summary.max = this_summary.max;
                new_summary.sum = this_summary.sum;

                combined_results.insert(std::pair(city, new_summary));
            }
            else
            {
                auto summary_so_far = tuple->second;
                summary_so_far.count += this_summary.count;
                if (this_summary.max > summary_so_far.max)
                {
                    summary_so_far.max = this_summary.max;
                }
                if (this_summary.min < summary_so_far.min)
                {
                    summary_so_far.min = this_summary.min;
                }
                summary_so_far.sum += this_summary.sum;
            }
        }
    }
}

int main()
{

    auto start = chrono::high_resolution_clock::now();

    auto [buffer, size] = mmap_file();

    const int n_threads = 1;
    std::future<std::unordered_map<City, Summary>> promises[n_threads];
    std::unordered_map<City, Summary> results[n_threads];

    int stepsize = size / n_threads;
    for (int i = 0; i < n_threads; i++)
    {
        promises[i] = std::async(std::launch::async, read_file, buffer, i * stepsize, (i + 1) * stepsize, 100);
    }

    // aggregate
    for (int i = 0; i < n_threads; i++)
    {
        results[i] = promises[i].get();
    }

    // std::unordered_map<City, Summary> combined_results = read_file(buffer, 0, size, 100);

    std::unordered_map<City, Summary> combined_results;
    combine_results(results, n_threads, combined_results);
    cout << "combined_results has size " << combined_results.size() << endl;

    // This prints ~400 lines and can be noisy; maybe comment it out.
    // aggregate(combined_results);

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "time elapsed (us): " << duration.count() << endl;
}
