
function a = cal_energy(tc,ps, RRC_in_t,drx_time,psm_time,TeDRX,ul_interval, dl_interval, evaltim,nj,gps)
    fileID = fopen(['td' int2str(tc) '.txt'],'w');

    Tp=0.00504;% Transmission time of packet
    NDL=10; %capacity of DL buffer
    NUL=10;%capacity of UL buffer
    TPSM= (psm_time-drx_time)/1000; % constant duration of PSM cycle if no UL packet is arriving
    TRRC = RRC_in_t/1000;  % length of RRC timer
    TDRX= TeDRX/1000;
    aDL= 1000/dl_interval;
    if ul_interval == 0
        ul_interval = 9999999999999999;
    end
    aUL= 1000/ul_interval;

    LimDRX=round(drx_time/TeDRX);

    if drx_time == 0
        LimDRX = 0;
    end

    voltage = 3.6;
    EPSM=0.000000003*voltage; % energy while in PSM
    ERRC=0.0088*voltage;% energy while in RRC
    EDRX=0.0006*voltage; % energy while in DRX
    EP=0.000141942384;% energy used due to paging
    ETRX=0.046*Tp*voltage ; % energy used to TX or RX a packet

    % arrival probabilities for PSM
    aULPSM(1)= exp(-aUL*TPSM);
    for j=2:NUL
        result=1;
        for k=1:j-1
            result=result*(aUL*TPSM)/k;
        end
        aULPSM(j)=result*exp(-aUL*TPSM);    
    end
    aULPSM(NUL+1)=1-sum(aULPSM(1:NUL));

    aDLPSM(1)= exp(-aDL*TPSM);
    for j=2:NDL
        result=1;
        for k=1:j-1
            result=result*(aDL*TPSM)/k;
        end
        aDLPSM(j)=result*exp(-aDL*TPSM);    
    end
    aDLPSM(NDL+1)=1-sum(aDLPSM(1:NDL));

    aS=aDL+aUL;
    aSPSM(1)= exp(-aS*TPSM);
    for j=2:NDL
        result=1;
        for k=1:j-1
            result=result*(aS*TPSM)/k;
        end
        aSPSM(j)=result*exp(-aS*TPSM);    
    end
    aSPSM(NDL+1)=1-sum(aSPSM(1:NDL));

    % arrival probabilities for DRX
    aULDRX(1)= exp(-aUL*TDRX);
    for j=2:NUL
        result=1;
        for k=1:j-1
            result=result*(aUL*TDRX)/k;
        end
        aULDRX(j)=result*exp(-aUL*TDRX);    
    end
    aULDRX(NUL+1)=1-sum(aULDRX(1:NUL));

    aDLDRX(1)= exp(-aDL*TDRX);
    for j=2:NDL
        result=1;
        for k=1:j-1
            result=result*(aDL*TDRX)/k;
        end
        aDLDRX(j)=result*exp(-aDL*TDRX);    
    end
    aDLDRX(NDL+1)=1-sum(aDLDRX(1:NDL));

    aS=aDL+aUL;
    aSDRX(1)= exp(-aS*TDRX);
    for j=2:NDL
        result=1;
        for k=1:j-1
            result=result*(aS*TDRX)/k;
        end
        aSDRX(j)=result*exp(-aS*TDRX);    
    end
    aSDRX(NDL+1)=1-sum(aSDRX(1:NDL));

    % arrival probabilities for RRC
    aULRRC(1)= exp(-aUL*TRRC);
    for j=2:NUL
        result=1;
        for k=1:j-1
            result=result*(aUL*TRRC)/k;
        end
        aULRRC(j)=result*exp(-aUL*TRRC);    
    end
    aULRRC(NUL+1)=1-sum(aULRRC(1:NUL));

    aDLRRC(1)= exp(-aDL*TRRC);
    for j=2:NDL
        result=1;
        for k=1:j-1
            result=result*(aDL*TRRC)/k;
        end
        aDLRRC(j)=result*exp(-aDL*TRRC);    
    end
    aDLRRC(NDL+1)=1-sum(aDLRRC(1:NDL));


    % transistion matrix P 
    dim=(NUL+1)*(NDL+1)+LimDRX;
    P=zeros(dim,dim);

    % transitions from PSM,0,0,0
    P(1,2)=aULPSM(1)*aDLPSM(1); % no DL of UL arrival during PSM

    %transitions to RRC,0,k_DL,0
    for i=1:NDL
        P(1,1+LimDRX+NUL+(NUL+1)*(i-1)+1)=aDLPSM(i+1)*aULPSM(1);
    end

    %transitions to RRC,1,k_DL,0
    for i=1:NDL-1
        P(1,1+LimDRX+NUL+(i-1)*(NUL+1)+2)=(aUL/aDL)*((aDL/aS)^(i+1))*(1-sum(aSPSM(1:i+1)));
        dd(i)=P(1,i*NUL+LimDRX+i+2);
    end
    P(1,NDL*(1+NUL)+LimDRX+2)=1-exp(-aUL*TPSM)-sum(dd(1:NDL-1))-(1-aULPSM(1))*aDLPSM(1);



    % transitions from DRX,0,0,tl_DRX

    % transistions from DRX,0,0,tl_DRX to DRX,0,0,tl_DRX+1
    for k=1:LimDRX-1
        P(k+1,k+2)=aULDRX(1)*aDLDRX(1);
    end

    %transision from DRX,0,0,Lim_DRX tot PSM,0,0,0
    P(LimDRX+1,1)=aULDRX(1)*aDLDRX(1);

    % Transitions from DRX,0,0,tl_DRX to RRC,0,k_DL,0
    for k=1:LimDRX
        for i=1:NDL
            P(k+1,1+LimDRX+NUL+(i-1)*(NUL+1)+1)=aULDRX(1)*aDLDRX(i+1);
        end
    end

    % Transitions from DRX,0,0,tl_DRX to RRC,1,k_DL,0
    for k=1:LimDRX
        P(1+k,LimDRX+2)=(1-aULDRX(1))*aDLDRX(1);
    end
    for k=1:LimDRX
        for i=1:NDL-1
            P(k+1,i*NUL+LimDRX+i+2)=(aUL/aDL)*((aDL/aS)^(i+1))*(1-sum(aSDRX(1:i+1)));
            cc(i)=P(k+1,i*NUL+LimDRX+i+2);
        end
        P(k+1,NDL*(1+NUL)+LimDRX+2)=1-exp(-aUL*TDRX)-sum(cc(1:NDL-1))-(1-aULDRX(1))*aDLDRX(1);
    end

    %Transitions from RRC,1 or 0,k_DL,0

    % transitions from RRC,0,k_DL,0
    for i=1:NDL
        P(LimDRX+1+NUL+(i-1)*(NUL+1)+1,2)=aULRRC(1)*aDLRRC(1);
        P(LimDRX+1+NUL+(i-1)*(NUL+1)+1,LimDRX+2)=(aUL/(aUL+aDL))*(1-exp(-(aUL+aDL)*TRRC));
        P(LimDRX+1+NUL+(i-1)*(NUL+1)+1,LimDRX+NUL+2)=(aDL/(aUL+aDL))*(1-exp(-(aUL+aDL)*TRRC));
    end

    %Transitions from RRC,1,k_DL,0
    P(1+LimDRX+1,2)=aULRRC(1)*aDLRRC(1);
    P(1+LimDRX+1,LimDRX+2)=(aUL/(aUL+aDL))*(1-exp(-(aUL+aDL)*TRRC));
    P(1+LimDRX+1,LimDRX+NUL+2)=(aDL/(aUL+aDL))*(1-exp(-(aUL+aDL)*TRRC));
    for i=1:NDL
        P(LimDRX+1+NUL+(i-1)*(NUL+1)+2,2)=aULRRC(1)*aDLRRC(1);
        P(LimDRX+1+NUL+(i-1)*(NUL+1)+2,LimDRX+2)=(aUL/(aUL+aDL))*(1-exp(-(aUL+aDL)*TRRC));
        P(LimDRX+1+NUL+(i-1)*(NUL+1)+2,LimDRX+NUL+2)=(aDL/(aUL+aDL))*(1-exp(-(aUL+aDL)*TRRC));
    end



    % Computation of the steady state distribution state
    [V,D,W]=eig(P);

    for i = 1:1:dim
        W(:,i) = W(:,i)/sum(W(:,i)); 
    end

    position = 0;
    for i= 1:dim
        if abs(D(i,i)-1.0)<0.0001
            position = i;
        end
    end

    state = transpose(W(:,position));

    % Computation of HOLDING times

    Holding=zeros(dim,1);
    % Holding time PSM
    aa=exp(-aUL*TPSM);
    Holding(1)=aa*TPSM+(1-aa)*(1/(aUL)-(1/(aUL))*aa-TPSM*aa);

    % Holding time DRX
    aa=exp(-aUL*TDRX);
    for k=1:LimDRX
        Holding(1+k)=aa*TDRX+(1-aa)*(1/(aUL)-(1/(aUL))*aa-TDRX*aa);
    end

    % Holding time RRC
    aTOT=aDL+aUL;
    BP(1)=(1/(1-aTOT*Tp))*Tp;
    for i=1:NDL
        BP0(i)=(i/(1-aTOT*Tp))*Tp;
        BP1(i)=((i+1)/(1-aTOT*Tp))*Tp;
    end
    dd=exp(-aTOT*TRRC);
    ee=exp(-aDL*TRRC);
    ff=exp(-aUL*TRRC);
    Holdingrest=(aUL/aTOT)*(1-dd)*(1/(aUL)-(1/(aUL))*ff-TRRC*ff)+(aDL/aTOT)*(1-dd)*(1/(aDL)-(1/(aDL))*ee-TRRC*ee)+dd*TRRC;
    Holding(LimDRX+2)=BP(1)+Holdingrest;
    for i=1:NDL
        Holding(1+LimDRX+NUL+(i-1)*(NUL+1)+1)=BP0(i)+Holdingrest;
        Holding(1+LimDRX+NUL+(i-1)*(NUL+1)+2)=BP1(i)+Holdingrest;
    end

    %computation of the probability to be in a state
    denom=0;
    for i=1:dim
        denom=denom+Holding(i)*state(i);
    end
    nu=zeros(dim,1);
    for i=1:dim
        nu(i)=Holding(i)*state(i)/denom;
    end

    totnuPSM=nu(1);
    totnuDRX=sum(nu(2:1+LimDRX));
    totnuRRC=sum(nu(2+LimDRX:dim));

    %Computation of the delay

    %Computation of delay of DL packets

    %Delay component PSM
    Delcomp=zeros(dim,1);
    %computation of the residual PSM time when a UL packet arrives
    aa=exp(-aUL*TPSM);
    mean=(1/(aUL)-(1/(aUL))*aa-TPSM*aa);
    secmom=-(TPSM^2)*aa-(2*TPSM/aUL)*aa-(2/(aUL^2))*aa+2/(aUL^2);
    variance=secmom-mean^2;
    residual=(mean^2+variance)/(2*mean);
    % Delay when DL arrives during PSM
    DPSM=aa*(TPSM/2+Tp*(aDL*TPSM/2+1))+(1-aa)*(residual+Tp*(aDL*residual+1));
    Delcomp(1)=DPSM;

    %Delay component DRX
    %computation of the residual DRX time when a UL packet arrives
    aa=exp(-aUL*TDRX);
    mean=(1/(aUL)-(1/(aUL))*aa-TDRX*aa);
    secmom=-(TDRX^2)*aa-(2*TDRX/aUL)*aa-(2/(aUL^2))*aa+2/(aUL^2);
    variance=secmom-mean^2;
    residual=(mean^2+variance)/(2*mean);
    % Delay when DL arrives during PSM
    DDRX=aa*(TDRX/2+Tp*(aDL*TDRX/2+1))+(1-aa)*(residual+Tp*(aDL*residual+1));
    for i=1:LimDRX
        Delcomp(2:i+1)=DDRX;
    end

    %Delay component RRC
    % kUL=0
    for i=1:NDL
        A1=(i/(1-aTOT*Tp))*Tp;
        A2= Holding(1+LimDRX+NUL+(i-1)*(NUL+1)+1)-A1;
        DRRC(1+LimDRX+NUL+(i-1)*(NUL+1)+1)=(A1/(A1+A2))*(i*(1-aTOT)*Tp+aTOT*Tp)*Tp/((2*(1-aTOT*Tp))^2)+(A2/(A1+A2))*Tp;
    end
    A1=(1/(1-aTOT*Tp))*Tp;
    A2= Holding(2+LimDRX)-A1;
    Delcomp(2+LimDRX)=(A1/(A1+A2))*(1*(1-aTOT)*Tp+aTOT*Tp)*Tp/((2*(1-aTOT*Tp))^2)+(A2/(A1+A2))*Tp;
    for i=1:NDL
        A1=((i+1)/(1-aTOT*Tp))*Tp;
        A2= Holding(1+LimDRX+NUL+(i-1)*(NUL+1)+2)-A1;
        Delcomp(1+LimDRX+NUL+(i-1)*(NUL+1)+2)=(A1/(A1+A2))*((i+1)*(1-aTOT)*Tp+aTOT*Tp)*Tp/((2*(1-aTOT*Tp))^2)+(A2/(A1+A2))*Tp;
    end

    % Total delay


    Delaytot=0;
    for i=1:dim
        Delaytot=Delaytot+nu(i)*Delcomp(i);
    end
    DelayDL=Delaytot;

    % ENERGY consumption

    energy=zeros(dim,1);
    energy(1)=nu(1)*EPSM;
    for i=1:LimDRX
        energy(1+i)=nu(i+1)*(EDRX+EP/Holding(i+1));
    end
    energy(LimDRX+2)=nu(LimDRX+2)*(ERRC+(1/(1-aTOT*Tp))*ETRX/Holding(LimDRX+2));
    for i=1:NDL
        energy(1+LimDRX+NUL+(i-1)*(NUL+1)+1)=nu(1+LimDRX+NUL+(i-1)*(NUL+1)+1)*(ERRC+(i/(1-aTOT*Tp))*ETRX/Holding(1+LimDRX+NUL+(i-1)*(NUL+1)+1));
        energy(1+LimDRX+NUL+(i-1)*(NUL+1)+2)=nu(1+LimDRX+NUL+(i-1)*(NUL+1)+2)*(ERRC+((1+i)/(1-aTOT*Tp))*ETRX/Holding(1+LimDRX+NUL+(i-1)*(NUL+1)+2));
    end
    Energytot=sum(energy(1:dim,1));
    fprintf(fileID,'AverageDLdelay: %f seconds for time %f \n',DelayDL, evaltim);
    fprintf(fileID,'AverageEnergy: %f J for time %f \n',Energytot*evaltim, evaltim);

    fclose(fileID);
end






