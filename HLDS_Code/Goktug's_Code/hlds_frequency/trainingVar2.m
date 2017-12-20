function trainingVar2(s, k, varOut, varState, path, epoch)

% global C H G F B D P R Q FJ HJ DG BF XJ
% s=3;   %dimension of the third layer states (z)
% k=10;  %dimension of the second layer states (u)
m=512;  %dimension  of the observation (y)
n=60; %dimension of the first layer states (x) %32 for 1st data %46 for piano

C=eye(s);        %state transition matrix of layer 3 states
% C=rand(s,s)-0.5;
H=givemeGammatoneFDomain(11025,n,2*m); %11025 for 1stdata 
G=rand(k,k)-0.5; %state transition matrix of layer 2 states
F=rand(n,n)-0.5; %state transition matrix of layer 1 states
% H=rand(m,n)-0.5; %measurement matrix

B=rand(n,k)-0.5; %inter-layer influence from second layer to first
D=rand(k,s)-0.5; %inter-layer influence from third layer to second

FJ=[C zeros(s,k) zeros(s,n); D G zeros(k,n) ; zeros(n,s) B,F]; %the joint state transition matrix
HJ=[zeros(m,s),zeros(m,k),H]; %the joint measurement matrix

Q=varState.*eye(k+n+s); %system transition uncertainty covariance for joint state
% Q=diag([0.1.*ones(1,s) 0.1.*ones(1,k) 0.1.*ones(1,n)]);
R=varOut*eye(m); %measurement eqn uncertainty covariance
p = triu(rand(k+n+s)-0.5);
P=p+p'-diag(diag(p)); %initialize the error covariance matrix for joint state
clear p

%initialize the error covariance matrices for parallelized parameter est.
for i=1:k
    PDG(1,i)={eye(k+s)};
end
for i=1:n
    PBF(1,i)={eye(k+n)};
end


load trumpet_iowa_cell
filename=strcat(path, ['training_results_s' num2str(s) '_k' num2str(k) ...
    '_varOut' num2str(varOut) '_varState' num2str(varState)  '.mat']);
collection = [collection(1:35)];
number_of_notes=length(collection);

seq = [randperm(number_of_notes),randperm(number_of_notes),randperm(number_of_notes),randperm(number_of_notes),randperm(number_of_notes),randperm(number_of_notes)];

for rep=1:epoch*number_of_notes
    rep
%     Y=prepFreqObservations(collection{ceil(number_of_notes*rand)},2*m,800); %800 for 1st data 3600 for piano
    Y=prepFreqObservations(collection{seq(rep)},2*m,800);
    X=zeros(n,size(Y,2));
    U=zeros(k,size(Y,2));
    Z=zeros(s,size(Y,2));

    tic
    for in=1:size(Y,2)
%         tic
        if in==1
            if rep==1
                [XJ,P]=state_estimate(FJ,HJ,P,R,Q,zeros(k+n+s,1),Y(:,in));
            else
                [XJ,P]=state_estimate(FJ,HJ,P,R,Q,XJ,Y(:,in));
            end
            Z(:,in)=XJ(1:s);
            U(:,in)=XJ(s+1:s+k);
            X(:,in)=XJ(s+k+1:s+k+n);
        else
            [XJ,P]=state_estimate(FJ,HJ,P,R,Q,XJ,Y(:,in)); %zeros(k+n+s,1)
            Z(:,in)=XJ(1:s);
            U(:,in)=XJ(s+1:s+k);
            X(:,in)=XJ(s+k+1:s+k+n);
            DG=[D,G];
            [DG,PDG]=parameter_estimate2(DG,[Z(:,in-1);U(:,in-1)],U(:,in),0.1,PDG);
            D=DG(:,1:s);
            G=DG(:,s+1:s+k);
            BF=[B,F];
            [BF,PBF]=parameter_estimate2(BF,[U(:,in-1);X(:,in-1)],X(:,in),0.1,PBF);
            B=BF(:,1:k);
            F=BF(:,k+1:k+n);
            FJ=[C zeros(s,k) zeros(s,n); D G zeros(k,n) ; zeros(n,s) B,F];

            if mod(in,10)==1
                subplot(3,3,1)
                plot(Y(:,in));
                hold on 
                plot(H*X(:,in),'r')
                hold off
                title('observation')
                subplot(3,3,3)
                imagesc(BF);
                colormap(gray)
                subplot(3,3,6)
                imagesc(DG);
                colormap(gray)
                subplot(334)
                plot(X(1,1:in)');
                title('state')
                subplot(337)
                plot(U(1,1:in)');
                title('second layer')
                subplot(332)
                if size(Z,1)==3
                    plot3(Z(1,1:in)',Z(2,1:in)',Z(3,1:in)');
                else
                    plot(Z(1,1:in)',Z(2,1:in)');
                end
                title('third layer')
                subplot(335)
                stem(X(:,in));
                title('state')
                subplot(338)
                stem(U(:,in));
                title('second layer')
                subplot(339)
                stem(Z(:,in));
                title('third layer')
                disp(['Rep: ' num2str(rep) ' Iteration ' num2str(in) ' time for last iteration:' num2str(toc)])
                pause(0.05)
            end
    
        end
        
    end


end

save(filename)

end
    
