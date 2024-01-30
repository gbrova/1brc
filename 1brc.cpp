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

void do_the_work(char *filepath)
{
    std::ifstream infile(filepath);

    std::unordered_map<std::string, Summary> temps;

    std::string line;
    while (std::getline(infile, line))
    {
        std::string city;
        std::string temp;

        // Lines are formatted as city;temperature
        int split_pos = line.find(";");

        city = line.substr(0, split_pos);
        int tempf = str_to_int(line, split_pos + 1);

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
