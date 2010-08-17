vs_1_1
dcl_position v0
dcl_color v1
dcl_normal v3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; c0 - c3 is view matrix           ;;
;; c12 is directional light vector  ;;
;; c13 is directional light color   ;;
;; c14 is diffuse coefficient       ;;
;; c15 is ambient light color       ;;
;; c16 is point light color         ;;
;; c17 is point light position      ;;
;; c18 are attenuation constants    ;;
;; c19 is specular coefficient      ;;
;; c20 is specular constant 'f'     ;;
;; c21 is eye position              ;;
;; c22 is spot light position       ;;
;; c23 is spot light color          ;;
;; c24 is spot light direction      ;;
;; c25 is 1/(IN - OUT)              ;;
;; c26 is OUT/(IN - OUT)            ;;
;; c27 - c30 is pos.*rot. matrix    ;;
;;                                  ;;
;; c100 is constant 0.0f            ;;
;; c111 is constant 1.0f            ;;
;;                                  ;;
; ?r0  is attenuation               ;;
; !r1  is transformed vertex        ;;
;; r2  is r (for specular)          ;;
;; r3  is temp                      ;;
;; r4  is light intensity           ;;
; !r5  is cos(theta)                ;;
; !r6  is result color              ;;
;; r7  is temp                      ;;
;; r8  is cos(phi) (specular)       ;;
; !r9  is normalized eye (v)        ;;
; !r10 is transformed normal        ;;
;; r11 is direction vector          ;;
;;                                  ;;
;; r0 is 1/current radius           ;;
;; r1 is a quotient                 ;;
;; r2 is a vertex after morphing    ;;
;;                                  ;;
; ?r0  is attenuation               ;;
; !r1  is transformed vertex        ;;
;; r2  is r (for specular)          ;;
;; r3  is temp                      ;;
;; r4  is light intensity           ;;
; !r5  is cos(theta)                ;;
; !r6  is result color              ;;
;; r7  is temp                      ;;
;; r8  is cos(phi) (specular)       ;;
; !r9  is normalized eye (v)        ;;
; !r10 is transformed normal        ;;
;; r11 is direction vector          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def c100, 0.0, 0.0, 0.0, 0.0
def c111, 1.0, 1.0, 1.0, 1.0 ;constant one

m4x4 r1, v0, c27        ; position and rotation

;;;;;;;;;;;;;;;;;;;;;;;; Results ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
m4x4 oPos, r1, c0
mov oD0, v1
