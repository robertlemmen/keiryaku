
(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))

(define rember-f
  (lambda (test? a l)
    (cond
      ((null? l) '())
      ((test? (car l) a) (cdr l))
      (else
        (cons (car l) (rember-f test? a (cdr l)))))))

(rember-f eq? 2 '(1 2 3 4 5))

(define eq?-c
  (lambda (a)
    (lambda (x)
      (eq? a x))))

((eq?-c 'tuna) 'tuna)
((eq?-c 'tuna) 'salad)

(define eq?-salad (eq?-c 'salad))

(eq?-salad 'salad)
(eq?-salad 'tuna)

(define rember-f
  (lambda (test?)
    (lambda (a l)
      (cond
        ((null? l) '())
        ((test? (car l) a) (cdr l))
        (else
          (cons (car l) ((rember-f test?) a (cdr l))))))))

((rember-f eq?) 2 '(1 2 3 4 5))

(define rember-eq? (rember-f eq?))

(rember-eq? 2 '(1 2 3 4 5))
(rember-eq? 'tuna '(tuna salad is good))
(rember-eq? 'tuna '(shrimp salad and tuna salad))
(rember-eq? 'eq? '(equal? eq? eqan? eqlist? eqpair?))

(define insertL-f
  (lambda (test?)
    (lambda (new old l)
      (cond
        ((null? l) '())
        ((test? (car l) old)
         (cons new (cons old (cdr l))))
        (else
          (cons (car l) ((insertL-f test?) new old (cdr l))))))))

((insertL-f eq?)
  'd
  'e
  '(a b c e f g d h))

(define insertR-f
  (lambda (test?)
    (lambda (new old l)
      (cond
        ((null? l) '())
        ((test? (car l) old)
         (cons old (cons new (cdr l))))
        (else
          (cons (car l) ((insertR-f test?) new old (cdr l))))))))

((insertR-f eq?)
  'e
  'd
  '(a b c d f g d h))

(define seqL
  (lambda (new old l)
    (cons new (cons old l))))

(define seqR
  (lambda (new old l)
    (cons old (cons new l))))

(define insert-g
  (lambda (seq)
    (lambda (new old l)
      (cond
        ((null? l) '())
        ((eq? (car l) old)
         (seq new old (cdr l)))
        (else
          (cons (car l) ((insert-g seq) new old (cdr l))))))))

(define insertL (insert-g seqL))

(define insertR (insert-g seqR))

(insertL
  'd
  'e
  '(a b c e f g d h))

(insertR
  'e
  'd
  '(a b c d f g d h))

(define insertL
  (insert-g
    (lambda (new old l)
      (cons new (cons old l)))))

(insertL
  'd
  'e
  '(a b c e f g d h))

(define subst-f
  (lambda (new old l)
    (cond
      ((null? l) '())
      ((eq? (car l) old)
       (cons new (cdr l)))
      (else
        (cons (car l) (subst new old (cdr l)))))))

(define seqS
  (lambda (new old l)
    (cons new l)))

(define subst (insert-g seqS))

(subst
  'topping
  'fudge
  '(ice cream with fudge for dessert))

(define yyy
  (lambda (a l)
    ((insert-g seqrem) #f a l)))

(define seqrem
  (lambda (new old l)
    l))

(yyy
  'sausage
  '(pizza with sausage and bacon))


(define value
  (lambda (nexp)
    (cond
      ((atom? nexp) nexp)
      ((eq? (car (cdr nexp)) 'o+)
       (+ (value (car nexp))
          (value (car (cdr (cdr nexp))))))
      ((eq? (car (cdr nexp)) 'o*)
       (* (value (car nexp))
          (value (car (cdr (cdr nexp))))))
      ((eq? (car (cdr nexp)) 'o^)
       (expt (value (car nexp))
             (value (car (cdr (cdr nexp))))))
      (else #f))))

(define atom-to-function
  (lambda (atom)
    (cond
      ((eq? atom 'o+) +)
      ((eq? atom 'o*) *)
      ((eq? atom 'o^) expt)
      (else #f))))

(define operator
  (lambda (aexp)
    (car aexp)))

(atom-to-function (operator '(o+ 5 3)))

(define value
  (lambda (nexp)
    (cond
      ((atom? nexp) nexp)
      (else
        ((atom-to-function (operator nexp))
         (value (1st-sub-exp nexp))
         (value (2nd-sub-exp nexp)))))))

(define 1st-sub-exp
  (lambda (aexp)
    (car (cdr aexp))))

(define 2nd-sub-exp
  (lambda (aexp)
    (car (cdr (cdr aexp)))))

(value 13)
(value '(o+ 1 3))
(value '(o+ 1 (o^ 3 4)))

(define multirember
  (lambda (a lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) a)
       (multirember a (cdr lat)))
      (else
        (cons (car lat) (multirember a (cdr lat)))))))

(define multirember-f
  (lambda (test?)
    (lambda (a lat)
      (cond
        ((null? lat) '())
        ((test? (car lat) a)
         ((multirember-f test?) a (cdr lat)))
        (else
          (cons (car lat) ((multirember-f test?) a (cdr lat))))))))

((multirember-f eq?) 'tuna '(shrimp salad tuna salad and tuna))

(define multirember-eq? (multirember-f eq?))

(define multiremberT
  (lambda (test? lat)
    (cond
      ((null? lat) '())
      ((test? (car lat))
       (multiremberT test? (cdr lat)))
      (else
        (cons (car lat)
              (multiremberT test? (cdr lat)))))))

(define eq?-tuna
  (eq?-c 'tuna))

(multiremberT
  eq?-tuna
  '(shrimp salad tuna salad and tuna))

(define multiremember&co
  (lambda (a lat col)
    (cond
      ((null? lat)
       (col '() '()))
      ((eq? (car lat) a)
       (multiremember&co a (cdr lat)
       (lambda (newlat seen)
         (col newlat (cons (car lat) seen)))))
      (else
        (multiremember&co a (cdr lat)
                          (lambda (newlat seen)
                            (col (cons (car lat) newlat) seen)))))))

(define a-friend
  (lambda (x y)
    (null? y)))

(multiremember&co
  'tuna
  '(strawberries tuna and swordfish)
  a-friend)
(multiremember&co
  'tuna
  '()
  a-friend)
(multiremember&co
  'tuna
  '(tuna)
  a-friend)

(define new-friend
  (lambda (newlat seen)
    (a-friend newlat (cons 'tuna seen))))

(multiremember&co
  'tuna
  '(strawberries tuna and swordfish)
  new-friend)
(multiremember&co
  'tuna
  '()
  new-friend)
(multiremember&co
  'tuna
  '(tuna)
  new-friend)

(define last-friend
  (lambda (x y)
    (length x)))

(multiremember&co
  'tuna
  '(strawberries tuna and swordfish)
  last-friend)
(multiremember&co
  'tuna
  '()
  last-friend)
(multiremember&co
  'tuna
  '(tuna)
  last-friend)


(define multiinsertLR
  (lambda (new oldL oldR lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) oldL)
       (cons new
             (cons oldL
                   (multiinsertLR new oldL oldR (cdr lat)))))
      ((eq? (car lat) oldR)
       (cons oldR
             (cons new
                   (multiinsertLR new oldL oldR (cdr lat)))))
      (else
        (cons
          (car lat)
          (multiinsertLR new oldL oldR (cdr lat)))))))

(multiinsertLR
  'x
  'a
  'b
  '(a o a o b o b b a b o))

(define multiinsertLR&co
  (lambda (new oldL oldR lat col)
    (cond
      ((null? lat)
       (col '() 0 0))
      ((eq? (car lat) oldL)
       (multiinsertLR&co new oldL oldR (cdr lat)
                         (lambda (newlat L R)
                           (col (cons new (cons oldL newlat))
                                (+ 1 L) R))))
      ((eq? (car lat) oldR)
       (multiinsertLR&co new oldL oldR (cdr lat)
                         (lambda (newlat L R)
                           (col (cons oldR (cons new newlat))
                                L (+ 1 R)))))
      (else
        (multiinsertLR&co new oldL oldR (cdr lat)
                          (lambda (newlat L R)
                            (col (cons (car lat) newlat)
                                 L R)))))))

(define col1
  (lambda (lat L R)
    lat))
(define col2
  (lambda (lat L R)
    L))
(define col3
  (lambda (lat L R)
    R))

(multiinsertLR&co
  'salty
  'fish
  'chips
  '(chips and fish or fish and chips)
  col1)
(multiinsertLR&co
  'salty
  'fish
  'chips
  '(chips and fish or fish and chips)
  col2)
(multiinsertLR&co
  'salty
  'fish
  'chips
  '(chips and fish or fish and chips)
  col3)

(define evens-only*
  (lambda (l)
    (cond
      ((null? l) '())
      ((atom? (car l))
       (cond
         ((even? (car l))
          (cons (car l)
                (evens-only* (cdr l))))
         (else
           (evens-only* (cdr l)))))
      (else
        (cons (evens-only* (car l))
              (evens-only* (cdr l)))))))

(evens-only*
  '((9 1 2 8) 3 10 ((9 9) 7 6) 2))

(define evens-only*&co
  (lambda (l col)
    (cond
      ((null? l)
       (col '() 1 0))
      ((atom? (car l))
       (cond
         ((even? (car l))
          (evens-only*&co (cdr l)
                          (lambda (newl p s)
                            (col (cons (car l) newl) (* (car l) p) s))))
         (else
           (evens-only*&co (cdr l)
                           (lambda (newl p s)
                             (col newl p (+ (car l) s)))))))
      (else
        (evens-only*&co (car l)
                        (lambda (al ap as)
                          (evens-only*&co (cdr l)
                                          (lambda (dl dp ds)
                                            (col (cons al dl)
                                                 (* ap dp)
                                                 (+ as ds))))))))))

(define evens-friend
  (lambda (e p s)
    e))

(evens-only*&co 
  '((9 1 2 8) 3 10 ((9 9) 7 6) 2)
  evens-friend)

(define evens-product-friend
  (lambda (e p s)
    p))

(evens-only*&co 
  '((9 1 2 8) 3 10 ((9 9) 7 6) 2)
  evens-product-friend)

(define evens-sum-friend
  (lambda (e p s)
    s))

(evens-only*&co 
  '((9 1 2 8) 3 10 ((9 9) 7 6) 2)
  evens-sum-friend)

(define the-last-friend
  (lambda (e p s)
    (cons s (cons p e))))

(evens-only*&co 
  '((9 1 2 8) 3 10 ((9 9) 7 6) 2)
  the-last-friend)

===
(1 3 4 5)
#t
#f
#t
#f
(1 3 4 5)
(1 3 4 5)
(salad is good)
(shrimp salad and salad)
(equal? eqan? eqlist? eqpair?)
(a b c d e f g d h)
(a b c d e f g d h)
(a b c d e f g d h)
(a b c d e f g d h)
(a b c d e f g d h)
(ice cream with topping for dessert)
(pizza with and bacon)
<?type 6>
13
4
82
(shrimp salad salad and)
(shrimp salad salad and)
#f
#t
#f
#f
#f
#f
3
0
0
(x a o x a o b x o b x b x x a b x o)
(chips salty and salty fish or salty fish and chips salty)
2
2
((2 8) 10 (() 6) 2)
((2 8) 10 (() 6) 2)
1920
38
(38 1920 (2 8) 10 (() 6) 2)

