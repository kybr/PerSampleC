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

" " autocmd TextChanged,TextChangedI *.c execute system('./per-sample-c-client', getline(0, line('$')))
" function! ShowPreProcessor()
"   let l:meh = system('tcc -c -', getline(0, line('$')))
" 
"   silent! bdelete! __C_ERR__
"   setlocal splitright
"   silent vsplit __C_ERR__
"   setlocal filetype=c
"   setlocal buftype=nofile
"   "wincmd l:nr
"   
"   normal! ggdG
"   call append(0, split(meh, '\v\n'))
"   wincmd p
" 
"   "let meh = system('tcc -E -|clang-format>.foo', getline(0, line('$')))
"   "pedit .foo
" endfunction
" 
" autocmd CursorHold,CursorHoldI *.c call ShowPreProcessor()
