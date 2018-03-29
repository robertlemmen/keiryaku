;; these are cases from "The little Schemer", taken and adapted from 
;; https://github.com/pkrumins/the-little-schemer.
(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))
(define lat?
  (lambda (l)
    (cond
      ((null? l) #t)
      ((atom? (car l)) (lat? (cdr l)))
      (else #f))))
(lat? '(Jack Sprat could eat no chicken fat))
(lat? '())
(lat? '(bacon and eggs))
(lat? '((Jack) Sprat could eat no chicken fat))
(lat? '(Jack (Sprat could) eat no chicken fat))
(lat? '(bacon (and eggs)))
(or (null? '()) (atom? '(d e f g)))
(or (null? '(a b c)) (null? '()))
(or (null? '(a b c)) (null? '(atom)))
(define member?
  (lambda (a lat)
    (cond
      ((null? lat) #f)
      (else (or (eq? (car lat) a)
                (member? a (cdr lat)))))))
(member? 'meat '(mashed potatoes and meat gravy))
(member? 'meat '(potatoes and meat gravy))
(member? 'meat '(and meat gravy))
(member? 'meat '(meat gravy))
(member? 'liver '(bagels and lox))
(member? 'liver '())
===
#t
#t
#t
#f
#f
#f
#t
#t
#f
#t
#t
#t
#t
#f
#f

