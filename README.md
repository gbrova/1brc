## 1brc

Solve the 1 billion row challenge (https://github.com/gunnarmorling/1brc) but slowly and badly.

## Testing

```bash
# Have the correct result, computed in python
python3 sol.py | sort > result_sorted_py.txt

# Compile and run
g++ -Wall -std=c++17 -O3 -g 1brc.cpp -o 1brc

#  this diff should be empty (except log messages etc)
./1brc | sort | diff result_sorted_py.txt - 
```
