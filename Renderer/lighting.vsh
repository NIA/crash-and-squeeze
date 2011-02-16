vs_1_1
dcl_position v0
dcl_color0 v1
dcl_color1 v2 ; cluster index
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
;; c31-c46 are initial cluster c.m. ;;
;; c47-c110 are cluster matrices    ;;
;;                                  ;;
;; c127 is constant 0.0f            ;;
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

def c127, 0.0, 0.0, 0.0, 0.0
def c111, 1.0, 1.0, 1.0, 1.0 ;constant one
def c114, 4.0, 4.0, 4.0, 4.0 ;constant 4

; - - - - - - - - - -  position  - - - - - - - - - - - - - -;
mov a0.x, v2.x          ; a0.x = cluster index
add r1, v0, -c[31+a0.x] ; r1 = vertex.pos - cluster.center
mul r3.x, v2.x, c114
mov a0.x, r3.x          ; a0.x = 4 * cluster index
m4x4 r2, r1, c[47+a0.x] ; r2 = [cluster.matrix] * r1
m4x4 r1, r2, c27        ; r1 = [position and rotation] * r2
; - - - - - - - - - -  normals  - - - - - - - - - - - - - - ;
m4x4 r10, v3, c27       ; position and rotation
dp3 r2, r10, r10        ; r2 = |normal|**2
rsq r7, r2              ; r7 = 1/|normal|
mul r10, r10, r7.x      ; normalize r10

; calculating normalized v
add r9, c21, -r1       ; r9 = position(eye) - position(vertex)
dp3 r0, r9, r9         ; r0 = distance**2
rsq r7, r0             ; r7 = 1/distance
mul r9, r9, r7.x       ; normalize r9
;;;;;;;;;;;;;;;;;;;;; Directional ;;;;;;;;;;;;;;;;;;;;;;;;;;;
dp3 r5, c12, r10        ; r5 = cos(theta)
; - - - - - - - - - - - diffuse - - - - - - - - - - - - - - ;
mul r4, c13, r5.x       ; r4 = I(direct)*cos(theta)
mul r4, r4, c14.x       ; r4 *= coef(diffuse)

max r6, r4, c127        ; if some color comp. < 0 => make it == 0
; - - - - - - - - - - - specular - - - - - - - - - - - - - -;
; calculating r:
mul r2, r10, r5.x     ; r2 = 2*(l, n)*n
add r2, r2, r2        ; r2 = 2*(l, n)*n
add r2, r2, -c12      ; r2 = 2*(l, n)*n - l
; calculating cos(phi)**f
dp3 r8, r2, r9          ; r8 = cos(phi)
max r8, r8, c127        ; if cos < 0, let it = 0
mov r7.y, r8.x
mov r7.w, c20.x
lit r8, r7              ; r8.z = cos(phi)**f

mul r4, c13, r8.z       ; r4 = I(direct)*cos(phi)**f
mul r4, r4, c19.x       ; r4 *= coef(specular)

max r4, r4, c127        ; if some color comp. < 0 => make it == 0
add r6, r6, r4

;;;;;;;;;;;;;;;;;;;;;;;; Point ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; calculating normalized direction vector
add r11, c17, -r1       ; r11 = position(point) - position(vertex)
dp3 r2, r11, r11        ; r2 = distance**2
rsq r7, r2              ; r7 = 1/distance
mul r11, r11, r7.x      ; normalize r11
; calculating cos(theta)
dp3 r5, r11, r10        ; r5 = cos(theta)
; calculating attenuation
dst r2, r2, r7          ; r2 = (1, d, d**2, 1/d)
dp3 r0, r2, c18         ; r0 = (a + b*d + c*d**2)
rcp r0, r0              ; r0 = attenuation coef
; - - - - - - - - - - - diffuse - - - - - - - - - - - - - - ;
mul r4, c16, r5.x       ; r4 = I(point)*cos(theta)
mul r4, r4, c14.x        ; r4 *= coef(diffuse)
mul r4, r4, r0.x        ; r4 *= attenuation

max r4, r4, c127        ; if some color comp. < 0 => make it == 0
add r6, r6, r4
; - - - - - - - - - - - specular - - - - - - - - - - - - - -;
; calculating r:
mul r2, r10, r5.x   ; r2 = (l, n)*n
add r2, r2, r2      ; r2 = 2*(l, n)*n
add r2, r2, -r11    ; r2 = 2*(l, n)*n - l
; calculating cos(phi)**f
dp3 r8, r2, r9          ; r8 = cos(phi)
max r8, r8, c127        ; if cos < 0, let it = 0
mov r7.y, r8.x
mov r7.w, c20.x
lit r8, r7              ; r8.z = cos(phi)**f

mul r4, c16, r8.z       ; r4 = I(point)*cos(phi)**f
mul r4, r4, c19.x       ; r4 *= coef(specular)
mul r4, r4, r0.x        ; r4 *= attenuation

max r4, r4, c127        ; if some color comp. < 0 => make it == 0
add r6, r6, r4

;;;;;;;;;;;;;;;;;;;;;;; Ambient ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
add r6, r6, c15         ; r6 += I(ambient)

;;;;;;;;;;;;;;;;;;;;;;;; Results ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
m4x4 oPos, r1, c0
mul oD0, v1, r6
