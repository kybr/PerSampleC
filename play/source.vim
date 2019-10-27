" Vim’s event model by Damian Conway (2010)
" https://developer.ibm.com/tutorials/l-vim-script-5/
"
" autocmd  EventName  filename_pattern   :command
"
function! TryCode()
  let l:message = system('./per-sample-c-client', getline(0, line('$')))

  :redraw!
"  :redrawstatus!
"  :redrawtabline

" echohl WarningMsg | echomsg "=> " . a:msg | echohl None
" echoerr 'oh it failed'
" echomsg 'hello there'
" echo 'hello'

  if l:message != ''
    echomsg 'Per-Sample C: ' . l:message
  else
    if v:shell_error != 0
      echomsg 'Per-Sample C: CRASH!'
    endif
  endif
endfunction

autocmd TextChanged,TextChangedI *.c :call TryCode()
