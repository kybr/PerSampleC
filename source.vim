autocmd TextChanged,TextChangedI *.c execute system('./per-sample-c-client', getline(0, line('$')))
