'marker
(eof-object? (eof-object))
(eof-object? 'test)
(define fh (open-input-file "t/01-basic/18-ports.t"))
(read fh)
(eof-object? (read fh))
(close-port fh)
===
marker
#t
#f
(quote marker)
#f
