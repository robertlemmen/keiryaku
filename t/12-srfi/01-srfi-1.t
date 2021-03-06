(include "srfi/srfi-1.ss")

; XXX fix the commented out ones

(cons 'a '())
(cons '(a) '(b c d))
(cons "a" '(b c))
(cons 'a 3)
(cons '(a b) 'c)
(list 'a (+ 3 4) 'c)
(list)
(xcons '(b c) 'a)
(cons* 1 2 3 4)
(cons* 1)
(make-list 4 'c)
;(list-tabulate 4 values)
(take (circular-list 'z 'q) 6)
;(iota 5)
;(let ((res (iota 5 0 -0.1)))
;    (cons (inexact->exact (car res)) (cdr res)))
(pair? '(a . b))
(pair? '(a b c))
(pair? '())
;(pair? '#(a b)) XXX should be false?
(pair? 7)
(pair? 'a)
;(list= eq?)
;(list= eq? '(a))
(car '(a b c))
(cdr '(a b c))
(car '((a) b c d))
(cdr '((a) b c d))
(car '(1 . 2))
(cdr '(1 . 2))
(list-ref '(a b c d) 2)
(third '(a b c d e))
(take '(a b c d e)  2)
(drop '(a b c d e)  2)
(take '(1 2 3 . d) 2)
(drop '(1 2 3 . d) 2)
(take '(1 2 3 . d) 3)
(drop '(1 2 3 . d) 3)
(take-right '(a b c d e) 2)
(drop-right '(a b c d e) 2)
(take-right '(1 2 3 . d) 2)
(drop-right '(1 2 3 . d) 2)
(take-right '(1 2 3 . d) 0)

(drop-right '(1 2 3 . d) 0)
(last '(a b c))
(last-pair '(a b c))
(append '(x) '(y))
(append '(a) '(b c d))
(append '(a (b)) '((c)))
(append '(a b) '(c . d))
(append '() 'a)
(append '(x y))
(append)
(reverse '(a b c))
(reverse '(a (b c) d (e (f))))
;(zip '(one two three) '(1 2 3) '(odd even odd even odd even odd even))
(zip '(1 2 3))
;(test '((3 #f) (1 #t) (4 #f) (1 #t)) (zip '(3 1 4 1) (circular-list #f #t)))
;(test-values (values '(1 2 3) '(one two three)) (unzip2 '((1 one) (2 two) (3 three))))
(count even? '(3 1 4 1 5 9 2 5 6))
;(count < '(1 2 4 8) '(2 4 6 8 10 12 14 16))
;(count < '(3 1 4 1) (circular-list 1 10))
;(fold cons* '() '(a b c) '(1 2 3 4 5))
;(test '(a 1 b 2 c 3) (fold-right cons* '() '(a b c) '(1 2 3 4 5)))
;(test '((a b c) (b c) (c)) (pair-fold-right cons '() '(a b c)))
;(test '((a b c) (1 2 3) (b c) (2 3) (c) (3)) (pair-fold-right cons* '() '(a b c) '(1 2 3)))
(map cadr '((a b) (d e) (g h)))
(map (lambda (n) (expt n n)) '(1 2 3 4 5))
;(map + '(1 2 3) '(4 5 6))
;(test '(4 1 5 1) (map + '(3 1 4 1) (circular-list 1 0)))
;(test '#(0 1 4 9 16) (let ((v (make-vector 5))) (for-each (lambda (i) (vector-set! v i (* i i))) '(0 1 2 3 4)) v))
;(test '(1 -1 3 -3 8 -8) (append-map (lambda (x) (list x (- x))) '(1 3 8)))
;(test '(1 -1 3 -3 8 -8) (apply append (map (lambda (x) (list x (- x))) '(1 3 8))))
;(test '(1 -1 3 -3 8 -8) (append-map! (lambda (x) (list x (- x))) '(1 3 8)))
;(test '(1 -1 3 -3 8 -8) (apply append! (map (lambda (x) (list x (- x))) '(1 3 8))))
;(test "pair-for-each-1" '((a b c) (b c) (c))
;      (let ((a '()))
;        (pair-for-each (lambda (x) (set! a (cons x a))) '(a b c))
;        (reverse a)))
(filter-map (lambda (x) (and (number? x) (* x x))) '(a 1 b 3 c 7))
(filter even? '(0 7 8 8 43 -4))
;(test-values (values '(one four five) '(2 3 6)) (partition symbol? '(one 2 3 four five 6)))
(remove even? '(0 7 8 8 43 -4))
(find even? '(1 2 3))
(any  even? '(1 2 3))
(find even? '(1 7 3))
(any  even? '(1 7 3))
;(test-error (any  even? '(1 3 . x)))
;(test '6 (find even? (circular-list 1 6 3)))
;(test '#t (any  even? (circular-list 1 6 3)))
(find even? '(3 1 4 1 5 9))
(find-tail even? '(3 1 37 -8 -5 0 0))
(find-tail even? '(3 1 37 -5))
(take-while even? '(2 18 3 10 22 9))
(drop-while even? '(2 18 3 10 22 9))
;(test-values (values '(2 18) '(3 10 22 9)) (span even? '(2 18 3 10 22 9)))
;(test-values (values '(3 1) '(4 1 5 9)) (break even? '(3 1 4 1 5 9)))
(any integer? '(a 3 b 2.7))
(any integer? '(a 3.1 b 2.7))
;(test '#t (any < '(3 1 4 1 5) '(2 7 1 8 2)))
(list-index even? '(3 1 4 1 5 9))
;(test '1 (list-index < '(3 1 4 1 5 9 2 5 6) '(2 7 1 8 2)))
;(test '#f (list-index = '(3 1 4 1 5 9 2 5 6) '(2 7 1 8 2)))
(memq 'a '(a b c))
(memq 'b '(a b c))
(memq 'a '(b c d))
(memq (list 'a) '(b (a) c))
(member (list 'a) '(b (a) c))
(memv 101 '(100 101 102))
;(delete-duplicates '(a b a c a b c z))
;(test '((a . 3) (b . 7) (c . 1)) (delete-duplicates '((a . 3) (b . 7) (a . 9) (c . 1)) (lambda (x y) (eq? (car x) (car y)))))
;(let ((e '((a 1) (b 2) (c 3))))
;  (test '(a 1) (assq 'a e))
;  (test '(b 2) (assq 'b e))
;  (test '#f (assq 'd e))
;  (test '#f (assq (list 'a) '(((a)) ((b)) ((c)))))
;  (test '((a)) (assoc (list 'a) '(((a)) ((b)) ((c)))))
;  (test '(5 7) (assv 5 '((2 3) (5 7) (11 13)))))
;(test '#t (lset<= eq? '(a) '(a b a) '(a b c c)))
;(test '#t (lset<= eq?))
;(test '#t (lset<= eq? '(a)))
;(test '#t (lset= eq? '(b e a) '(a e b) '(e e b a)))
;(test '#t (lset= eq?))
;(test '#t (lset= eq? '(a)))
;(test '(u o i a b c d c e) (lset-adjoin eq? '(a b c d c e) 'a 'e 'i 'o 'u))
;(test '(u o i a b c d e) (lset-union eq? '(a b c d e) '(a e i o u)))
;(test '(x a a c) (lset-union eq? '(a a c) '(x a x)))
;(test '() (lset-union eq?))
;(test '(a b c) (lset-union eq? '(a b c)))
;(test '(a e) (lset-intersection eq? '(a b c d e) '(a e i o u)))
;(test '(a x a) (lset-intersection eq? '(a x y a) '(x a x z)))
;(test '(a b c) (lset-intersection eq? '(a b c)))
;(test '(b c d) (lset-difference eq? '(a b c d e) '(a e i o u)))
;(test '(a b c) (lset-difference eq? '(a b c)))
;(test #t (lset= eq? '(d c b i o u) (lset-xor eq? '(a b c d e) '(a e i o u))))
;(test '() (lset-xor eq?))
;(test '(a b c d e) (lset-xor eq? '(a b c d e)))
===
(a)
((a) b c d)
("a" b c)
(a . 3)
((a b) . c)
(a 7 c)
()
(a b c)
(1 2 3 . 4)
1
(c c c c)
(z q z q z q)
#t
#t
#f
#f
#f
a
(b c)
(a)
(b c d)
1
2
c
c
(a b)
(c d e)
(1 2)
(3 . d)
(1 2 3)
d
(d e)
(a b c)
(2 3 . d)
(1)
d
(1 2 3)
c
(c)
(x y)
(a b c d)
(a (b) (c))
(a b c . d)
a
(x y)
()
(c b a)
((e (f)) d (b c) a)
((1) (2) (3))
3
(b e h)
(1 4 27 256 3125)
(1 9 49)
(0 8 8 -4)
(7 43)
2
#t
#f
#f
4
(-8 -5 0 0)
#f
(2 18)
(3 10 22 9)
#t
#f
2
(a b c)
(b c)
#f
#f
((a) c)
(101 102)
