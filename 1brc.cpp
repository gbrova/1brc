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

// kcachegrind callgrind.out.11921 and before that, valgrind --tool=callgrind ./helloworld
// or try gprof

void parse()
{
    // TODO: split out the huge function, so I can understand where the time is going.
}

void aggregate(std::unordered_map<std::string, Summary> &temps)
{
    // for (std::unordered_map<std::string, Summary>::iterator iter = temps.begin(); iter != temps.end(); ++iter)
    for (const auto &[city, thisSummary] : temps)
    {
        // std::string city = iter->first;
        // Summary thisSummary = iter->second;

        float min = (float)thisSummary.min / 10;
        float max = (float)thisSummary.max / 10;
        float avg = (float)thisSummary.sum / thisSummary.count / 10;

        fmt::print("{}={:.1f}/{:.1f}/{:.1f}\n", city, min, max, avg);
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

int str_to_int(std::string line, int start_pos)
{
    int tempf = 0;
    int sign = 1;
    for (int i = start_pos; i < line.length(); i++)
    {
        char c = line.at(i);
        if (c == '.')
        {
            continue;
        }
        if (c == '-')
        {
            sign = -1;
            continue;
        }
        int d = c - '0';
        tempf = tempf * 10 + d;
    }
    return tempf * sign;
}

int str_to_int2(std::vector<char> &buffer, int start_pos, int end_pos)
{
    // cout << start_pos << ", " << end_pos << endl;
    int tempf = 0;
    int sign = 1;
    for (int i = start_pos; i < end_pos; i++)
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
        int d = c - '0';
        tempf = tempf * 10 + d;
    }
    return tempf * sign;
}

int read_city(char city[], char *buffer, int start_pos)
{
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

const int CITY_LENGTH = 32;

class City
{

public:
    char city[CITY_LENGTH];

    City(char city_c[])
    {
        memcpy(city, city_c, CITY_LENGTH);
    }

    bool operator==(const City &other) const
    {
        // return (city == other.city);
        return memcmp(city, other.city, CITY_LENGTH);
    }

    // std::size_t operator()(const City &s) const noexcept
    // {
    //     return 0;
    //     // std::size_t h1 = std::hash<std::string>{}(s.first_name);
    //     // std::size_t h2 = std::hash<std::string>{}(s.last_name);
    //     // return h1 ^ (h2 << 1); // or use boost::hash_combine
    // }
};
namespace std
{
    template <>
    struct hash<City>
    {
        size_t operator()(const City &city) const
        {
            // Compute individual hash values for two data members and combine them using XOR and bit shifting
            // return ((hash<float>()(k.getM()) ^ (hash<float>()(k.getC()) << 1)) >> 1);
            // return 0;

            // hash<string> hasher;
            // size_t hash = hasher(city.city);

            // size_t result;
            // memcpy(&result, city.city, 8);

            // return (int)city.city;

            // return *(size_t *)city.city;

            // return result;
            // int sum = city.city[2] + city.city[2];
            // int foo = city.city[0] * 977 * 1129 + city.city[1] * 727 + city.city[2];
            // int foo = 0;
            return city.city[0] << 24 + city.city[1] << 16 + city.city[2] << 8 + city.city[3];
            // return foo;

            // return city.city[0] << 16 + city.city[1] << 8 + city.city[2];
            // return city.city.length();
            // return hash;
        }
    };
}

void remember(char city[], int tempf, std::unordered_map<City, Summary> &temps)
{
    // this lookup makes it take 42ms (i.e. +20ms), which seems like a lot
    City city_class = City(city);

    auto tuple = temps.find(city_class);

    // everything together is 56ms (i.e. +14ms), which is maybe reasonable
    if (tuple == temps.end())
    {
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
        // only reading takes 12ms
        // so reading+copying takes 24ms, which is about as good as we can do (we basically copy everything once).
        i = read_city(city, buffer, i);

        auto [ii, tempf] = read_number(buffer, i);
        i = ii;

        remember(city, tempf, temps);
    }

    return temps;
}

std::tuple<char *, int> mmap_file()
{

    char *filepath = "./measurements.txt.1m";
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

    // int sum = 0;
    // for (int i = 0; i < statbuf.st_size; i++)
    // {
    //     sum += buffer[i];
    // }
    // cout << "got " << sum << endl;

    return {buffer, statbuf.st_size};
}

void combine_results(std::unordered_map<std::string, Summary> results[], int n_results, std::unordered_map<std::string, Summary> combined_results)
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

                // cout << "here" << endl;
                // cout << city << endl;
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

    // std::unordered_map<std::string, Summary> combined_results;
    // combine_results(results, n_threads, combined_results);
    // aggregate(combined_results);

    // or this

    // int stepsize = size / 1;
    // cout << "before" << endl;
    // auto future1 = std::async(read_file, 0, stepsize, 100);
    // std::cout << "start" << endl;
    // std::unordered_map<std::string, Summary> temps1 = future1.get();

    // or this.

    // std::unordered_map<std::string, Summary> temps = read_file(0, size / 10, 100);

    // TODO(sort first)
    // aggregate(temps1);

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << duration.count() << endl;
}
