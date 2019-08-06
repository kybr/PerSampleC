autocmd TextChanged,TextChangedI *.c execute system('tcc -E - | clang-format | ./per-sample-c-client', getline(0, line('$')))
