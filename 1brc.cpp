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
    float sum;
    float min;
    float max;
};

void aggregate(std::map<std::string, Summary> &temps)
{
    // For simplicity/laziness, we're not sorting the result. But the time taken to sort ~400 rows is trivial, so ignore it for now.
    for (std::map<std::string, Summary>::iterator iter = temps.begin(); iter != temps.end(); ++iter)
    {
        std::string city = iter->first;
        Summary thisSummary = iter->second;

        float avg = thisSummary.sum / thisSummary.count;

        fmt::print("{}={:.1f}/{:.1f}/{:.1f}\n", city, thisSummary.min, thisSummary.max, avg);
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

void parse_line(std::string line)
{
}

void do_the_work(char *filepath)
{
    std::ifstream infile(filepath);

    std::map<std::string, Summary> temps;

    std::string line;
    while (std::getline(infile, line))
    {
        std::string city;
        std::string temp;

        // Lines are formatted as city;temperature
        int split_pos = line.find(";");

        city = line.substr(0, split_pos);
        temp = line.substr(split_pos + 1);

        float tempf = std::stof(temp);

        if (temps.find(city) == temps.end())
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
            Summary &thisSummary = temps.find(city)->second;
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
