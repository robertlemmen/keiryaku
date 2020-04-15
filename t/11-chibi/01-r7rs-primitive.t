;; these are taken and adapted from 
;; https://github.com/gypified/chibi-scheme/blob/master/tests/r7rs-tests.scm
;; XXX this should be converted to the "test" framework used by chibi once
;; we can actually run that (needs define-syntax et al)

(let ()
    (define x 28)
    x)
(quote a)
(quote #(a b c))
(quote (+ 1 2))
'a
'#(a b c)
'()
'(+ 1 2)
'(quote a)
''a
'"abc"
"abc"
'145932
145932
'#t
#t
(+ 3 4)
((if #f + *) 3 4)
((lambda (x) (+ x x)) 4)
(define reverse-subtract
    (lambda (x y) (- y x)))
(reverse-subtract 7 10)
(define add4
    (let ((x 4))
        (lambda (y) (+ x y))))
(add4 6)
((lambda x x) 3 4 5 6)
((lambda (x y . z) z)
    3 4 5 6)
(if (> 3 2) 'yes 'no)
(if (> 2 3) 'yes 'no)
(if (> 3 2)
    (- 3 2)
    (+ 3 2))
(let ()
    (define x 2)
    x)
===
28
a
(vector a b c)
(+ 1 2)
a
(vector a b c)
()
(+ 1 2)
(quote a)
(quote a)
"abc"
"abc"
145932
145932
#t
#t
7
12
8
3
10
(3 4 5 6)
(5 6)
yes
no
1
2

