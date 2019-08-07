" tcc -E -
"   filter through the preprocessor
"   removes comments, include files
"
" clang-format
"   parse and reconstitute source
"   removes/normalizes any/all formating
"
" ./per-sample-c-client
"   submit code to the server
"
autocmd TextChanged,TextChangedI *.c execute system('tcc -E - | clang-format | ./per-sample-c-client', getline(0, line('$')))
" autocmd TextChanged,TextChangedI *.c execute system('./per-sample-c-client', getline(0, line('$')))
