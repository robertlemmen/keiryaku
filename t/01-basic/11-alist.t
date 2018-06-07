;; taken and modified from https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Association-Lists.html
(define e '((a 1) (b 2) (c 3)))
(assq 'a e)
(assq 'b e)
(assq 'd e) 
(assq (list 'a) '(((a)) ((b)) ((c))))
(assoc (list 'a) '(((a)) ((b)) ((c))))
(assv 5 '((2 3) (5 7) (11 13)))
===
(a 1)
(b 2)
#f
#f
((a))
(5 7)
