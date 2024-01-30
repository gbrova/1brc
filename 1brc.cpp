#include <iostream>
#include <vector>
#include <string>

#include <sstream>
#include <string>
#include <fstream>
#include <map>

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

void aggregate(std::unordered_map<std::string, Summary> &temps)
{
    // For simplicity/laziness, we're not sorting the result. But the time taken to sort ~400 rows is trivial, so ignore it for now.
    for (const auto &[city, thisSummary] : temps)
    {
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

void save_temperature(char city[], int tempf, std::unordered_map<std::string, Summary> &temps)
{
    auto tuple = temps.find(city);

    if (tuple == temps.end())
    {
        // not found
        Summary thisSummary;
        thisSummary.count = 1;
        thisSummary.min = tempf;
        thisSummary.max = tempf;
        thisSummary.sum = tempf;
        temps.insert(std::pair(city, thisSummary));
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

void do_the_work(char *filepath)
{
    std::unordered_map<std::string, Summary> temps;

    auto [buffer, size] = mmap_file(filepath);
    char city[100];

    for (std::size_t i = 0; i < size; i++)
    {
        i = read_city(city, buffer, i);
        auto [ii, tempf] = read_number(buffer, i);
        i = ii;

        save_temperature(city, tempf, temps);
    }

    aggregate(temps);
}

int main()
{
    auto start = chrono::high_resolution_clock::now();

    do_the_work("measurements.txt.1m");

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "time elapsed (us): " << duration.count() << endl;
}
