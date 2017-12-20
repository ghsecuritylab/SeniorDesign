function [class_rate1,class,true_label,SNR]=classification_given_centers_SNR_var2(centers,noi,s,k,varOut,varState, path)

% %trumpet
filename=strcat(path, ['training_results_s' num2str(s) '_k' num2str(k) ...
    '_varOut' num2str(varOut) '_varState' num2str(varState)  '.mat']);
%H:\Hierarchical Music\saved_training_polyphonic\

%saxophone
% filename=['H:\Hierarchical Music\saved_training_sax\training_results_s' num2str(s) '_k' num2str(k) '.mat'];
% number_of_notes=32;

load(filename)
% collection = [collection(1:35),collection(37:end)];
number_of_notes=length(collection);
clear Y
Y=[];
in1=1;
%prepare the observation sequence and true labels
sequence=1:number_of_notes;
sequence=sequence(randperm(number_of_notes));

noscol=[];
sndcol=[];
for ij=sequence
    nos=noi.*randn(size(collection{ij}));
    noscol=[noscol; nos];
    snd=collection{ij};
    sndcol=[sndcol; snd];
    addi=prepFreqObservations(snd+nos,2*m,800);
    Y=[Y,addi(:,1:size(addi,2)-10)];
    in2=size(Y,2);
    true_label(in1:in2)=repmat([ij],1,in2-in1+1);
    in1=in2+1;
end
SNR=10*log10(sum(abs(sndcol).^2)/sum(abs(noscol).^2));

%infer the states for the observation sequence
X=zeros(n,size(Y,2));
U=zeros(k,size(Y,2));
Z=zeros(s,size(Y,2));

tic
lim = 0;
for in=1:size(Y,2)
    progress = in*100/size(Y,2);
    if progress >= lim
        clc;
        disp(['Progress: ' num2str(lim) '%'])
        toc
        lim = lim + 0.5;
    end

    if in==1
        [XJ,P]=state_estimate(FJ,HJ,P,R,Q,zeros(k+n+s,1),Y(:,in));
        Z(:,in)=XJ(1:s);
        U(:,in)=XJ(s+1:s+k);
        X(:,in)=XJ(s+k+1:s+k+n);
    else
        for reprep=1:4
            [XJ,P]=state_estimate(FJ,HJ,P,R,Q,XJ,Y(:,in));%XJ
        end
        Z(:,in)=XJ(1:s);
        U(:,in)=XJ(s+1:s+k);
        X(:,in)=XJ(s+k+1:s+k+n);
    end
    
end


Y=[];
lag=1;
Yold=zeros(1,lag);

for ij=sequence
    addi=prepFreqObservations(collection{ij},2*m,800);
    Y=[Y,addi(:,1:size(addi,2)-10)];
    in1=size(Yold,2)+lag;
    in2=size(Y,2);
    tocluster{ij}=Z(:,in1:in2);
    Yold=Y;
end

for in=1:size(Y,2)
    current=repmat(Z(:,in)',number_of_notes,1);
    distances=sum((current-centers').^2,2);
    [val,class(in)]=min(distances);
end

class_rate1=sum(class==true_label)/size(Y,2);
% disp(['Classification Rate with all time indices: ' num2str(class_rate1*100)])
    
end