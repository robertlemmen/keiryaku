(define add1
  (lambda (n) (+ n 1)))
(add1 67)
(define sub1
  (lambda (n) (- n 1)))
(sub1 5)
(zero? 0)
(zero? 1492)
(define o+
  (lambda (n m)
    (cond
      ((zero? m) n)
      (else (add1 (o+ n (sub1 m)))))))
(o+ 46 12)
(define o-
  (lambda (n m)
    (cond
      ((zero? m) n)
      (else (sub1 (o- n (sub1 m)))))))
(o- 14 3)
(o- 17 9)
'(2 111 3 79 47 6)
'(8 55 5 555)
'()
'(1 2 8 apple 4 3)
'(3 (7 4) 13 9)
(define addtup
  (lambda (tup)
    (cond
      ((null? tup) 0)
      (else (o+ (car tup) (addtup (cdr tup)))))))
(addtup '(3 5 2 8))
(addtup '(15 6 7 12 3))
(define o*
  (lambda (n m)
    (cond
      ((zero? m) 0)
      (else (o+ n (o* n (sub1 m)))))))
(o* 5 3)
(o* 13 4)
(define tup+
  (lambda (tup1 tup2)
    (cond
      ((null? tup1) tup2)
      ((null? tup2) tup1)
      (else
        (cons (o+ (car tup1) (car tup2))
              (tup+ (cdr tup1) (cdr tup2)))))))
(tup+ '(3 6 9 11 4) '(8 5 2 0 7))
(tup+ '(3 7) '(4 6 8 1))
(define o>
  (lambda (n m)
    (cond
      ((zero? n) #f)
      ((zero? m) #t)
      (else
        (o> (sub1 n) (sub1 m))))))
(o> 12 133)
(o> 120 11)
(o> 6 6)
(define o<
  (lambda (n m)
    (cond
      ((zero? m) #f)
      ((zero? n) #t)
      (else
        (o< (sub1 n) (sub1 m))))))
(o< 4 6)
(o< 8 3)
(o< 6 6)
(define o=
  (lambda (n m)
    (cond
      ((o> n m) #f)
      ((o< n m) #f)
      (else #t))))
(o= 5 5)
(o= 1 2)
(define o^
  (lambda (n m)
    (cond
      ((zero? m) 1)
      (else (o* n (o^ n (sub1 m)))))))
(o^ 1 1)
(o^ 2 3)
(o^ 5 3)
(define o/
  (lambda (n m)
    (cond
      ((o< n m) 0)
      (else (add1 (o/ (o- n m) m))))))
(o/ 15 4)
(define olength
  (lambda (lat)
    (cond
      ((null? lat) 0)
      (else (add1 (olength (cdr lat)))))))
(olength '(hotdogs with mustard sauerkraut and pickles))
(olength '(ham and cheese on rye))
(define pick
  (lambda (n lat)
    (cond
      ((zero? (sub1 n)) (car lat))
      (else
        (pick (sub1 n) (cdr lat))))))
(pick 4 '(lasagna spaghetti ravioli macaroni meatball))
(define rempick
  (lambda (n lat)
    (cond
      ((zero? (sub1 n)) (cdr lat))
      (else
        (cons (car lat) (rempick (sub1 n) (cdr lat)))))))
(rempick 3 '(hotdogs with hot mustard))
(define no-nums
  (lambda (lat)
    (cond
      ((null? lat) '())
      ((number? (car lat)) (no-nums (cdr lat)))
      (else
        (cons (car lat) (no-nums (cdr lat)))))))
(no-nums '(5 pears 6 prunes 9 dates))
(define all-nums
  (lambda (lat)
    (cond
      ((null? lat) '())
      ((number? (car lat)) (cons (car lat) (all-nums (cdr lat))))
      (else
        (all-nums (cdr lat))))))
(all-nums '(5 pears 6 prunes 9 dates))
(define eqan?
  (lambda (a1 a2)
    (cond
      ((and (number? a1) (number? a2)) (= a1 a2))
      ((or  (number? a1) (number? a2)) #f)
      (else
        (eq? a1 a2)))))
(eqan? 3 3)
(eqan? 3 4)
(eqan? 'a 'a)
(eqan? 'a 'b)
(define occur
  (lambda (a lat)
    (cond
      ((null? lat) 0)
      ((eq? (car lat) a)
       (add1 (occur a (cdr lat))))
      (else
        (occur a (cdr lat))))))
(occur 'x '(a b x x c d x))
(occur 'x '())
(define one?
  (lambda (n) (= n 1)))
(one? 5)
(one? 1)
(define rempick-one
  (lambda (n lat)
    (cond
      ((one? n) (cdr lat))
      (else
        (cons (car lat) (rempick-one (sub1 n) (cdr lat)))))))
(rempick-one 4 '(hotdogs with hot mustard))
===
68
4
#t
#f
58
11
8
(2 111 3 79 47 6)
(8 55 5 555)
()
(1 2 8 apple 4 3)
(3 (7 4) 13 9)
18
43
15
52
(11 11 11 11 11)
(7 13 8 1)
#f
#t
#f
#t
#f
#f
#t
#f
1
8
125
3
6
5
macaroni
(hotdogs with mustard)
(pears prunes dates)
(5 6 9)
#t
#f
#t
#f
3
0
#f
#t
(hotdogs with hot)
