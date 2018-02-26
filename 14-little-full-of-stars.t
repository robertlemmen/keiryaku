(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))
(define add1
  (lambda (n) (+ n 1)))
(define rember*
  (lambda (a l)
    (cond
      ((null? l) '())
      ((atom? (car l))
       (cond
         ((eq? (car l) a)
          (rember* a (cdr l)))
         (else
           (cons (car l) (rember* a (cdr l))))))
      (else
        (cons (rember* a (car l)) (rember* a (cdr l)))))))
(rember*
  'cup
  '((coffee) cup ((tea) cup) (and (hick)) cup))
(rember*
  'sauce
  '(((tomato sauce)) ((bean) sauce) (and ((flying)) sauce)))
(define insertR*
  (lambda (new old l)
    (cond
      ((null? l) '())
      ((atom? (car l))
       (cond
         ((eq? (car l) old)
          (cons old (cons new (insertR* new old (cdr l)))))
         (else
           (cons (car l) (insertR* new old (cdr l))))))
      (else
        (cons (insertR* new old (car l)) (insertR* new old (cdr l)))))))
(insertR*
  'roast
  'chuck
  '((how much (wood)) could ((a (wood) chuck)) (((chuck)))
    (if (a) ((wood chuck))) could chuck wood))
(define occur*
  (lambda (a l)
    (cond
      ((null? l) 0)
      ((atom? (car l))
       (cond
         ((eq? (car l) a)
          (add1 (occur* a (cdr l))))
         (else
           (occur* a (cdr l)))))
      (else
        (+ (occur* a (car l))
           (occur* a (cdr l)))))))
(occur*
  'banana
  '((banana)
    (split ((((banana ice)))
            (cream (banana))
            sherbet))
    (banana)
    (bread)
    (banana brandy)))
(define subst*
  (lambda (new old l)
    (cond
      ((null? l) '())
      ((atom? (car l))
       (cond
         ((eq? (car l) old)
          (cons new (subst* new old (cdr l))))
         (else
           (cons (car l) (subst* new old (cdr l))))))
      (else
        (cons (subst* new old (car l)) (subst* new old (cdr l)))))))
(subst*
  'orange
  'banana
  '((banana)
    (split ((((banana ice)))
            (cream (banana))
            sherbet))
    (banana)
    (bread)
    (banana brandy)))
(define insertL*
  (lambda (new old l)
    (cond
      ((null? l) '())
      ((atom? (car l))
       (cond
         ((eq? (car l) old)
          (cons new (cons old (insertL* new old (cdr l)))))
         (else
           (cons (car l) (insertL* new old (cdr l))))))
      (else
        (cons (insertL* new old (car l)) (insertL* new old (cdr l)))))))
(insertL*
  'pecker
  'chuck
  '((how much (wood)) could ((a (wood) chuck)) (((chuck)))
    (if (a) ((wood chuck))) could chuck wood))
(define leftmost
  (lambda (l)
    (cond
      ((atom? (car l)) (car l))
      (else (leftmost (car l))))))
(leftmost '((potato) (chips ((with) fish) (chips))))
(leftmost '(((hot) (tuna (and))) cheese))
(define eqlist?
  (lambda (l1 l2)
    (cond
      ((and (null? l1) (null? l2)) #t)
      ((and (null? l1) (atom? (car l2))) #f)
      ((null? l1) #f)
      ((and (atom? (car l1)) (null? l2)) #f)
      ((and (atom? (car l1)) (atom? (car l2)))
       (and (eq? (car l1) (car l2))
            (eqlist? (cdr l1) (cdr l2))))
      ((atom? (car l1)) #f)
      ((null? l2) #f)
      ((atom? (car l2)) #f)
      (else
        (and (eqlist? (car l1) (car l2))
             (eqlist? (cdr l1) (cdr l2)))))))
(eqlist?
  '(strawberry ice cream)
  '(strawberry ice cream))
(eqlist?
  '(strawberry ice cream)
  '(strawberry cream ice))
(eqlist?
  '(banan ((split)))
  '((banana) split))
(eqlist?
  '(beef ((sausage)) (and (soda)))
  '(beef ((salami)) (and (soda))))
(eqlist?
  '(beef ((sausage)) (and (soda)))
  '(beef ((sausage)) (and (soda))))
(define eqlist2?
  (lambda (l1 l2)
    (cond
      ((and (null? l1) (null? l2)) #t)
      ((or (null? l1) (null? l2)) #f)
      ((and (atom? (car l1)) (atom? (car l2)))
       (and (eq? (car l1) (car l2))
            (eqlist2? (cdr l1) (cdr l2))))
      ((or (atom? (car l1)) (atom? (car l2)))
       #f)
      (else
        (and (eqlist2? (car l1) (car l2))
             (eqlist2? (cdr l1) (cdr l2)))))))
(eqlist2?
  '(strawberry ice cream)
  '(strawberry ice cream))
(eqlist2?
  '(strawberry ice cream)
  '(strawberry cream ice))
(eqlist2?
  '(banan ((split)))
  '((banana) split))
(eqlist2?
  '(beef ((sausage)) (and (soda)))
  '(beef ((salami)) (and (soda))))
(eqlist2?
  '(beef ((sausage)) (and (soda)))
  '(beef ((sausage)) (and (soda))))
(define equal??
  (lambda (s1 s2)
    (cond
      ((and (atom? s1) (atom? s2))
       (eq? s1 s2))
      ((atom? s1) #f)
      ((atom? s2) #f)
      (else (eqlist? s1 s2)))))
(equal?? 'a 'a)
(equal?? 'a 'b)
(equal?? '(a) 'a)
(equal?? '(a) '(a))
(equal?? '(a) '(b))
(equal?? '(a) '())
(equal?? '() '(a))
(equal?? '(a b c) '(a b c))
(equal?? '(a (b c)) '(a (b c)))
(equal?? '(a ()) '(a ()))
(define equal2??
  (lambda (s1 s2)
    (cond
      ((and (atom? s1) (atom? s2))
       (eq? s1 s2))
      ((or (atom? s1) (atom? s2)) #f)
      (else (eqlist? s1 s2)))))
(equal2?? 'a 'a)
(equal2?? 'a 'b)
(equal2?? '(a) 'a)
(equal2?? '(a) '(a))
(equal2?? '(a) '(b))
(equal2?? '(a) '())
(equal2?? '() '(a))
(equal2?? '(a b c) '(a b c))
(equal2?? '(a (b c)) '(a (b c)))
(equal2?? '(a ()) '(a ()))
(define eqlist3?
  (lambda (l1 l2)
    (cond
      ((and (null? l1) (null? l2)) #t)
      ((or (null? l1) (null? l2)) #f)
      (else
        (and (equal2?? (car l1) (car l2))
             (equal2?? (cdr l1) (cdr l2)))))))
(eqlist3?
  '(strawberry ice cream)
  '(strawberry ice cream))
(eqlist3?
  '(strawberry ice cream)
  '(strawberry cream ice))
(eqlist3?
  '(banan ((split)))
  '((banana) split))
(eqlist3?
  '(beef ((sausage)) (and (soda)))
  '(beef ((salami)) (and (soda))))
(eqlist3?
  '(beef ((sausage)) (and (soda)))
  '(beef ((sausage)) (and (soda))))
(define rember
  (lambda (s l)
    (cond
      ((null? l) '())
      ((equal2?? (car l) s) (cdr l))
      (else (cons (car l) (rember s (cdr l)))))))
(rember
  '(foo (bar (baz)))
  '(apples (foo (bar (baz))) oranges))
===
((coffee) ((tea)) (and (hick)))
(((tomato)) ((bean)) (and ((flying))))
((how much (wood)) could ((a (wood) chuck roast)) (((chuck roast))) (if (a) ((wood chuck roast))) could chuck roast wood)
5
((orange) (split ((((orange ice))) (cream (orange)) sherbet)) (orange) (bread) (orange brandy))
((how much (wood)) could ((a (wood) pecker chuck)) (((pecker chuck))) (if (a) ((wood pecker chuck))) could pecker chuck wood)
potato
hot
#t
#f
#f
#f
#t
#t
#f
#f
#f
#t
#t
#f
#f
#t
#f
#f
#f
#t
#t
#t
#t
#f
#f
#t
#f
#f
#f
#t
#t
#t
#t
#f
#f
#f
#t
(apples oranges)
