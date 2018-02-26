(define rember
  (lambda (a lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) a) (cdr lat))
      (else (cons (car lat)
                  (rember a (cdr lat)))))))
(rember 'mint '(lamb chops and mint flavored mint jelly))
(rember 'toast '(bacon lettuce and tomato))
(rember 'cup '(coffee cup tea cup and hick cup))
(define firsts
  (lambda (l)
    (cond
      ((null? l) '())
      (else
        (cons (car (car l)) (firsts (cdr l)))))))
(firsts '((apple peach pumpkin)
          (plum pear cherry)
          (grape raisin pea)
          (bean carrot eggplant)))
(firsts '((a b) (c d) (e f)))
(firsts '((five plums) (four) (eleven green oranges)))
(firsts '(((five plums) four)
          (eleven green oranges)
          ((no) more)))
(define insertR
  (lambda (new old lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) old)
       (cons old (cons new (cdr lat))))
      (else
        (cons (car lat) (insertR new old (cdr lat)))))))
(insertR
  'topping 'fudge
  '(ice cream with fudge for dessert))
(insertR
  'jalapeno
  'and
  '(tacos tamales and salsa))
(insertR
  'e
  'd
  '(a b c d f g d h))
(define insertL
  (lambda (new old lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) old)
       (cons new (cons old (cdr lat))))
      (else
        (cons (car lat) (insertL new old (cdr lat)))))))
(insertL
  'd
  'e
  '(a b c e g d h))
(define subst
  (lambda (new old lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) old)
       (cons new (cdr lat)))
      (else
        (cons (car lat) (subst new old (cdr lat)))))))
(subst
  'topping
  'fudge
  '(ice cream with fudge for dessert))
(define subst2
  (lambda (new o1 o2 lat)
    (cond
      ((null? lat) '())
      ((or (eq? (car lat) o1) (eq? (car lat) o2))
       (cons new (cdr lat)))
      (else
        (cons (car lat) (subst new o1 o2 (cdr lat)))))))
(subst2
  'vanilla
  'chocolate
  'banana
  '(banana ice cream with chocolate topping))
(define multirember
  (lambda (a lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) a)
       (multirember a (cdr lat)))
      (else
        (cons (car lat) (multirember a (cdr lat)))))))
(multirember
  'cup
  '(coffee cup tea cup and hick cup))
(define multiinsertR
  (lambda (new old lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) old)
       (cons old (cons new (multiinsertR new old (cdr lat)))))
      (else
        (cons (car lat) (multiinsertR new old (cdr lat)))))))
(multiinsertR
  'x
  'a
  '(a b c d e a a b))
(define multiinsertL
  (lambda (new old lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) old)
       (cons new (cons old (multiinsertL new old (cdr lat)))))
      (else
        (cons (car lat) (multiinsertL new old (cdr lat)))))))
(multiinsertL
  'x
  'a
  '(a b c d e a a b))
(define multisubst
  (lambda (new old lat)
    (cond
      ((null? lat) '())
      ((eq? (car lat) old)
       (cons new (multisubst new old (cdr lat))))
      (else
        (cons (car lat) (multisubst new old (cdr lat)))))))
(multisubst
  'x
  'a
  '(a b c d e a a b))
===
(lamb chops and flavored mint jelly)
(bacon lettuce and tomato)
(coffee tea cup and hick cup)
(apple plum grape bean)
(a c e)
(five four eleven)
((five plums) eleven (no))
(ice cream with fudge topping for dessert)
(tacos tamales and jalapeno salsa)
(a b c d e f g d h)
(a b c d e g d h)
(ice cream with topping for dessert)
(vanilla ice cream with chocolate topping)
(coffee tea and hick)
(a x b c d e a x a x b)
(x a b c d e x a x a b)
(x b c d e x x b)
