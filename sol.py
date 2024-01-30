from collections import defaultdict

cities = defaultdict(list)

for line in open('measurements.txt.1m'):
    city, temp = line.strip().split(';')
    cities[city].append(temp)


def summarize(temps):
    temps = [float(temp) for temp in temps]
    return min(temps), max(temps), sum(temps)/len(temps)

for city in sorted(cities.keys()):
    mi, ma, av = summarize(cities[city])
    print(f'{city}={mi:.1f}/{ma:.1f}/{av:.1f}')
