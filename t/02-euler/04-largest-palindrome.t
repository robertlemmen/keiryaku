; Problem 4: Largest palindrome product

; A palindromic number reads the same both ways. The largest palindrome made
; from the product of two 2-digit numbers is 9009 = 91 × 99.

; Find the largest palindrome made from the product of two 3-digit numbers.

; XXX make these helpers let-scoped
(define (number->revdigits x)
  (if (< x 10)
    (list x)
    (cons (remainder x 10)(number->revdigits (/ x 10)))))

(define (pali-number? x)
  (let ([rd (number->revdigits x)])
    (equal? rd (reverse rd))))

(define (innerf num seq lim)
  (if (null? seq)
    0
    (let ([cprod (* num (car seq))])
      (if (< cprod lim)
        lim
        (if (pali-number? cprod)
          cprod
          (innerf num (cdr seq) lim))))))

; XXX should be in base and take any number or args
(define (max a b)
  (if (> a b)
    a
    b))

(define (outerf seqa seqb lim)
  (if (null? seqa)
    0
    (let* ([ra (innerf (car seqa) seqb lim)]
           [rb (outerf (cdr seqa) seqb ra)])
      (max ra rb))))
    
; 906609 is correct
(outerf (iota 999 999 -1) (iota 999 999 -1) 0)



; XXX like this I can get the *parser* to segfault !!!
; (define (outerf seqa seqb lim)
;   (if (null? seqa)
;     0
;     (let* ([ra (innerf (car seqa) seqb lim)]
;            [rb (outerf (cdr seqa) seqb lim)])
;       (max ra rb))))
;     
; (outerf (iota 99 99 -1) (iota 99 99 -1) 0)
;
===
906609
