
      subroutine fixinv15(x,w,rold,rinv,asave,corr,ish)

c!!!  as in oneinv, but with a fixed GIVEN foc mech  !!!
c                   (which means enough changes!!!!
c                    besides other, no DC constraint is needed)

c Input: single elemse and 'data' (ori or residual)
c Temporal shift of elemse (in loop); finding moment tensor for each step
cfinding correlation for each step
c Output: iseqm number of loop steps
c Output: array: ish(1,... iseqm) = shifts
c Output: array: corr(1,... iseqm) = correlation
c Output: array: asave(1,... iseqm) = moment tensor coefficients

      dimension a(6),g(6),rinv(6,6),rold(6,6)
      dimension x(8192,21,3)
      dimension w(-3500:11692,21,3,6)
c     dimension anew(6),dela(6)
      dimension corr(100),ish(100),asave(6,100)
      dimension ntm(21)
      dimension weig(21,3)
      logical stat(21)


      common /NUMBERS/ nr,ntim,nmom,isubmax,ifirst,istep,ilast,
     *                 ff1(21),ff2(21),ff3(21),ff4(21),dt

      common /ST/ stat,ntm
      common /WEI/ weig

c **************************************************************************
C ** FOCAL MECHANISM (if a priori known and held fixed for all subevents) **
c **************************************************************************
c                  It looks crazy to calculate a's on each subr. call,
c                  would seem better to put in main and transfer by common,
c                  but the intention was to keep main unchanged !!!!

        do i=1,4
        read(200,*)
        enddo
        read(200,*) strike,dip,rake
        REWIND (200)

 
       if(strike.eq.0)   strike=0.1         ! NEW July 5, 2014; to prevent problems with zero MT components
       if(strike.eq.180) strike=180.1
       if(dip.eq.90)     dip=89.9
       if(rake.eq.0)     rake=0.1
       if(rake.eq.180)   rake=179.9
      


        xmoment=1.        !!!!!!!!!!!!!!!!!!!!!!!!!!!
        pi=3.1415927
 	strike=strike*pi/180.
 	dip=dip*pi/180.
 	rake=rake*pi/180.
 	sd=sin(dip)
 	cd=cos(dip)
 	sp=sin(strike)
 	cp=cos(strike)
 	sl=sin(rake)
 	cl=cos(rake)
 	s2p=2.*sp*cp
 	s2d=2.*sd*cd
 	c2p=cp*cp-sp*sp
 	c2d=cd*cd-sd*sd

 	xx1 =-(sd*cl*s2p + s2d*sl*sp*sp)*xmoment     ! Mxx
 	xx2 = (sd*cl*c2p + s2d*sl*s2p/2.)*xmoment    ! Mxy
 	xx3 =-(cd*cl*cp  + c2d*sl*sp)*xmoment        ! Mxz
 	xx4 = (sd*cl*s2p - s2d*sl*cp*cp)*xmoment     ! Myy
 	xx5 =-(cd*cl*sp  - c2d*sl*cp)*xmoment        ! Myz
 	xx6 =             (s2d*sl)*xmoment           ! Mzz

 	a(1) = xx2
 	a(2) = xx3
 	a(3) =-xx5
 	a(4) = (-2.*xx1 + xx4 + xx6)/3.
 	a(5) = (xx1 -2*xx4 + xx6)/3.
      a(6)=0.



c   	a(6) = (xx1+xx4+xx6)/3. ! for the above x's this a(6)=0.
c            ! but for a given non-DC MT (x1-x6), a(6) may be nonzero



c
c     COMPUTING 'POWER' OF THE DATA (observed or residual)
c           (used stations ONLY !)

      rr=0.
      do icom=1,3                                       !icom seismo comp.
        do ir=1,nr                                      ! ir =station
        if(stat(ir)) then
          do itim=1,ntim                                   ! itim=time
          xweig=x(itim,ir,icom)*weig(ir,icom)
          rr=rr+xweig*xweig
          enddo
        endif
        enddo
      enddo
      rr=rr*dt



C *********************************************************************
C ********NON-LINEAR PART; THE SHIFT LOOP = GRID SEARCH OVER SHIFT ****
C *********************************************************************

c
c     STARTING  LOOP on SHIFT
c       (time shift = ishift*dt; shift > 0 ... to the RIGHT)

      iseqm=(ilast-ifirst) / istep
      ishift=ifirst

      do 10 iseq=1,iseqm
      ishift=ishift+istep  ! value of the shift

c
c     FORMING RIGHT-HAND SIDE OF THE SYSTEM (dependent on the SHIFT)
c


      do it=1,nmom                                        !n moment tensor comp
      g(it)=0.                                            !it time
        do icom=1,3                                       !icom seismo comp.
          do ir=1,nr                                      !ir station
          if(stat(ir)) then
             do itim=1,ntim
             xweig=x(itim,ir,icom) * weig(ir,icom)**2.
             g(it)=g(it)+
     *            w(itim-ishift,ir,icom,it)*xweig * 1.e20
c                                                  1.e20 = FORMAL SCALING
c                     (compensating the previous scaling of the system matrix)
c               ! this MINUS ishift in w is equivalent to PLUS in x
c               ! this MINUS ishift=10 in w is means +10*dt in x, to the RIGHT
             enddo
          endif
          enddo
        enddo
      g(it)=g(it)*dt
      enddo




      sum1=0.
      do n=1,nmom
      sum1=sum1 + a(n)*g(n)/1.e20    ! g de-scaled by 1.e20
      enddo

      sum2=0.
      do n=1,nmom
      do m=1,nmom
      sum2=sum2 + rold(n,m)*a(n)*a(m)/1.e20  ! rold de-scaled
      enddo
      enddo



      xmom=sum1/sum2 

c simple 'scalar' analogy of inversion rinv*G = (1/Rnm)Gm
c ratio between scalar products obs*synt and synt*synt, where synt is for unit moment 
c See Eq. (9) of Zahradnik and Gallovic, JGR 115, B09310, 2010
c The resulting moment tensor will be product of the unitary moment tensor (for fixed mechan) and xmom
c From this definition it is possible that xmom<0, see below (does not mean negative scalar moment!).        

c       write(2345,*) xmom,sum1  ! to show possible cases of  xmom<0 and sum1<0 


      sum1=xmom*sum1
      sum2=xmom*xmom*sum2

c       write(5432,*) xmom,sum1  ! to show possible cases of  xmom<0 and THE REDEFINED sum1>0 


c            Since now already we have 'normal' situation, sum1=sum2,
c            thus delta (L2 fit) and psi (correlation) are computed normally,
c            i.e. using sum1 only.
c        Beware: a() is still for moment=1. Bellow will be multipl. by moment.

c                   **************************************************



c    delta (=indirect L2 misfit) =  error2 (direct), see check later

c     delta=rr - 2.*sum1 + sum2   !
      delta=rr -    sum1          ! the same (since sum1=sum2)


      psi = sum1 / rr 			  ! = 1- (delta/rr)...correlation squared
      
	  if(psi.lt.0) then			  ! only formal to allow taking sqrt
	  psi=0.
	  else
	  psi=sqrt(psi)   ! psi = correlation
      endif

       if(xmom.lt.0.) then
       psi=0. !!!!!!! IMPORTANT, see explanation below added Oct 5, 2014.		
       xmom=-1. * xmom
       endif

	   
c Xmom can be <0 and psi still >0, see above write(5432). 
c xmom<0 has nothing to do with negative scalar moment!
c Instead MT we work with wights a(n), n=1,2,...6  of 6 elementary MT's. 
c The a(n) are for unit moment, while a(n).xmom are for non-unit moment; 
c i.e., xmom includes size and sign of MT. 
c The case of xmom<0 corresponds to spontaneous change of the prescribed mechanism
c It occurs in order to flip sign of all synth waveforms which then correlate with data.
c In other words, while having no correlation with the prescribed mechanism, the mechanism
c having rake substituted by rak-180 will provide correlation between synth and obs data.
c This can be documented by commenting the line  ' if(xmom.lt.0.) psi=0.'.
c Since we do NOT want such a flip (e.g. when we know that the prescribed rake 
c is consistent with polarities) we put psi=0 in the case of xmom<0. This prevents
c the spontaneous change of the rake and signalizes zero correlation. 
c The variance reduction calculated directly from waveforms will be negative.



c =====================================================================
c       NO DC constraint here in this subroutine (opposed to oneinv)
c =====================================================================

      corr(iseq)=psi    ! saving correlation; iseq counts the shifts
      ish(iseq)=ishift

      do n=1,6              ! saving  MOMENT COEFFICIENTS
      asave(n,iseq)=a(n)*xmom 
      enddo

   10 continue      ! SHIFT LOOP FINISHED

      RETURN
      END

