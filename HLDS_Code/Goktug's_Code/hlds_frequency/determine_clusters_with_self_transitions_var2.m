function [centers] = ...
    determine_clusters_with_self_transitions_var2(s, k, varOut, ...
                                                    varState, path)

%trumpet

filename=strcat(path, ['training_results_s' num2str(s) '_k' num2str(k) ...
    '_varOut' num2str(varOut) '_varState' num2str(varState)  '.mat']);
%H:\Hierarchical Music\saved_training_polyphonic\
load(filename)
number_of_notes=length(collection);
clear Y
Y=[];


sequence = [];
for i = 1:number_of_notes
    sequence = [sequence, i, i];
end

for ij=sequence
    addi=prepFreqObservations(collection{ij},2*m,800);
    Y=[Y,addi(:,1:size(addi,2)-10)];
end

%infer the states for the observation sequence
Z=zeros(s,size(Y,2));

tic
lim = 0.01;
for in=1:size(Y,2)
    progress = in*100/size(Y,2);
    if progress >= lim
        disp(['Progress: ' num2str(lim) '%'])
        toc
        lim = lim + 0.5;
    end
    if in==1
        [XJ,P]=state_estimate(FJ,HJ,P,R,Q,zeros(k+n+s,1),Y(:,in));
        Z(:,in)=XJ(1:s);
    else
        for reprep=1:4
            [XJ,P]=state_estimate(FJ,HJ,P,R,Q,XJ,Y(:,in));%XJ
        end
        Z(:,in)=XJ(1:s);
%         scatter3(Z(1,1:in),Z(2,1:in),Z(3,1:in));
    end
    
    
end


Y=[];
lag=1;
Yold=zeros(1,lag);
tocluster = cell(number_of_notes,1);

for ij=sequence
    addi=prepFreqObservations(collection{ij},2*m,800);
    Y=[Y,addi(:,1:size(addi,2)-10)];
    in1=size(Yold,2)+lag;
    in2=size(Y,2);
    tocluster{ij}=[tocluster{ij}, Z(:,in1:in2)];
    Yold=Y;
end

for i = 1:number_of_notes
    centers(:,i)=mean(tocluster{i},2);
end
    
end