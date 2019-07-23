autocmd TextChanged,TextChangedI *.c execute system('./per-sample-c-submit', getline(0, line('$')))
