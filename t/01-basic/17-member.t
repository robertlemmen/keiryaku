(memq 'a '(a b c))
(memq 'b '(a b c))
(memq 'c '(a b c))
(memq 'd '(a b c))
(memq 'a '())
(memq '() '())
(memq '(b) '((a) (b) (c)))

; XXX cases for memv

(member 'a '(a b c))
(member 'b '(a b c))
(member 'c '(a b c))
(member 'd '(a b c))
(member 'a '())
(member '() '())
(member '(b) '((a) (b) (c)))

(member '(b) '((a) (b) (c)) equal?)

; XXX cases with custom predicate
===
(a b c)
(b c)
(c)
#f
#f
#f
#f
(a b c)
(b c)
(c)
#f
#f
#f
((b) (c))
((b) (c))
