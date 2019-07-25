autocmd TextChanged,TextChangedI *.c execute system('clang-format | ./per-sample-c-client', getline(0, line('$')))
