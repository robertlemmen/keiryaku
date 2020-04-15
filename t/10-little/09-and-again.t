;; these are cases from "The little Schemer", taken and adapted from 
;; https://github.com/pkrumins/the-little-schemer.
(define pick
  (lambda (n lat)
    (cond
      ((zero? (sub1 n)) (car lat))
      (else
        (pick (sub1 n) (cdr lat))))))
(define keep-looking
  (lambda (a sorn lat)
    (cond
      ((number? sorn)
       (keep-looking a (pick sorn lat) lat))
      (else (eq? sorn a )))))
(define looking
  (lambda (a lat)
    (keep-looking a (pick 1 lat) lat)))
(looking 'caviar '(6 2 4 caviar 5 7 3))
(looking 'caviar '(6 2 grits caviar 5 7 3))
(define eternity
  (lambda (x)
    (eternity x)))
(define first
  (lambda (p)
    (car p)))
(define second
  (lambda (p)
    (car (cdr p))))
(define build
  (lambda (s1 s2)
    (cons s1 (cons s2 '()))))
(define shift
  (lambda (pair)
    (build (first (first pair))
      (build (second (first pair))
        (second pair)))))
(shift '((a b) c))
(shift '((a b) (c d)))
(define a-pair?
  (lambda (x)
    (cond
      ((atom? x) #f)
      ((null? x) #f)
      ((null? (cdr x)) #f)
      ((null? (cdr (cdr x))) #t)
      (else #f))))
(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))
(define align
  (lambda (pora)
    (cond
      ((atom? pora) pora)
      ((a-pair? (first pora))
       (align (shift pora)))
      (else (build (first pora)
              (align (second pora)))))))
(define length*
  (lambda (pora)
    (cond
      ((atom? pora) 1)
      (else
        (+ (length* (first pora))
           (length* (second pora)))))))
(define weight*
  (lambda (pora)
    (cond
      ((atom? pora) 1)
      (else
        (+ (* (weight* (first pora)) 2)
           (weight* (second pora)))))))
(weight* '((a b) c))
(weight* '(a (b c)))
(define revpair
  (lambda (p)
    (build (second p) (first p))))
(define shuffle
  (lambda (pora)
    (cond
      ((atom? pora) pora)
      ((a-pair? (first pora))
       (shuffle (revpair pora)))
      (else
        (build (first pora)
          (shuffle (second pora)))))))
(shuffle '(a (b c)))
(shuffle '(a b))
; XXX something is broken here, this seems to recurse too deep...
;(shuffle '((a b) (c d)))
(define one?
  (lambda (n) (= n 1)))
(define C
  (lambda (n)
    (cond
      ((one? n) 1)
      (else
        (cond
          ((even? n) (C (/ n 2)))
          (else
            (C (add1 (* 3 n)))))))))
(define A
  (lambda (n m)
    (cond
      ((zero? n) (add1 m))
      ((zero? m) (A (sub1 n) 1))
      (else
        (A (sub1 n)
           (A n (sub1 m)))))))
(A 1 0)
(A 1 1)
(A 2 2)
(lambda (l)
  (cond
    ((null? l) 0)
    (else
      (add1 (eternity (cdr l))))))
(lambda (l)
  (cond
    ((null? l) 0)
    (else
      (add1
        ((lambda(l)
           (cond
             ((null? l) 0)
             (else
               (add1 (eternity (cdr l))))))
         (cdr l))))))
((lambda (length)
   (lambda (l)
     (cond
       ((null? l) 0)
       (else (add1 (length (cdr l)))))))
 eternity)
((lambda (f)
   (lambda (l)
     (cond
       ((null? l) 0)
       (else (add1 (f (cdr l)))))))
 ((lambda (g)
    (lambda (l)
      (cond
        ((null? l) 0)
        (else (add1 (g (cdr l)))))))
  eternity))
(lambda (mk-length)
  (mk-length eternity))
((lambda (mk-length)
   (mk-length mk-length))
 (lambda (mk-length)
   (lambda (l)
     (cond
       ((null? l) 0)
       (else
         (add1
           ((mk-length eternity) (cdr l))))))))
(((lambda (mk-length)
   (mk-length mk-length))
 (lambda (mk-length)
   (lambda (l)
     (cond
       ((null? l) 0)
       (else
         (add1
           ((mk-length mk-length) (cdr l))))))))
 '(1 2 3 4 5))
((lambda (mk-length)
   (mk-length mk-length))
 (lambda (mk-length)
   ((lambda (length)
      (lambda (l)
        (cond
          ((null? l) 0)
          (else
            (add1 (length (cdr l)))))))
    (lambda (x)
      ((mk-length mk-length) x)))))
((lambda (le)
   ((lambda (mk-length)
      (mk-length mk-length))
    (lambda (mk-length)
      (le (lambda (x)
            ((mk-length mk-length) x))))))
 (lambda (length)
   (lambda (l)
     (cond
       ((null? l) 0)
       (else (add1 (length (cdr l))))))))
(lambda (le)
  ((lambda (mk-length)
     (mk-length mk-length))
   (lambda (mk-length)
     (le (lambda (x)
           ((mk-length mk-length) x))))))
(define Y
  (lambda (le)
    ((lambda (f) (f f))
     (lambda (f)
       (le (lambda (x) ((f f) x)))))))
===
#t
#f
(a (b c))
(a (b (c d)))
7
5
(a (b c))
(a b)
2
3
7
<?type 9>
<?type 9>
<?type 9>
<?type 9>
<?type 9>
<?type 9>
5
<?type 9>
<?type 9>
<?type 9>
