clear all
close all
clc

% Training for multi-layered HLDS
% Time domain approach
% Point by point filtering
% Observation/pre-processing filters
% NO VISUALIZATION

% HDLS parameters
Fs = 11025;                       % sampling frequency
s = 3;                          % dimension of the third layer states (z)
k = 10;                         % dimension of the second layer states (u)
n = 60;                         % dimension of the first layer states (x) - also number of filters for this case
m = n;                          % dimension  of the observation (y)
%alph = 0.01;                    % IIR mean estimator parameter
alph = 0.005;
tr = 1050;                      % Training epochs

% Noise parameters
r = 0.1;                        % covariance parameter for parameter/matrices estimation
Q = 0.01*eye(k+n+s);             % process noise covariance matrix
R = 0.5;                        % measurement noise variance
P = eye(k+n+s);                 % Initialization of covariance matrix for joint state

% Filter bank
fn = n;
M = 1000;
op = 2;
[H1,h] = givemeGammatoneFDomain(Fs,fn,fft_s,M,op);

% Initializations for state-layer
C = eye(s);                     % state transition matrix of layer 3 - forces clustering and stability
G = rand(k,k) - 0.5;            % initial state transition matrix of layer 2
G1 = G;
D = rand(k,s) - 0.5;            % intial inter-layer influence from third to second layer
D1 = D;
F = rand(n,n) - 0.5;            % intial state transition matrix of layer 1
F1 = F;
B = rand(n,k) - 0.5;            % initial inter-layer influence from second to first layer
B1 = B;

% Observation-layer for time domain approach
H = eye(m);

% Initialization of the error covariance matrices for parallelized parameter estimation
for i=1:k
    PDG(1,i)={eye(k+s)};
end

for i=1:n
    PBF(1,i)={eye(k+n)};
end

% Joint state transition matrix
FJ = [C zeros(s,k) zeros(s,n); D G zeros(k,n) ; zeros(n,s) B,F]; 
% Joint measurement matrix
HJ = [zeros(m,s) zeros(m,k) H]; 

% TRAINING
% --------
load('trumpet_iowa_cell.mat')
col =  collection(:,1:35);
display('Training has started')
% No visualization needed
musict = zeros(0,0);
epoch_idx = zeros(1,tr);
for i = 1:tr
    music_sel = randi([1 35]);
    auxx = (col{music_sel})';
    aux_idx = round(length(auxx)/2);
    auxx = auxx(aux_idx-1000:aux_idx+1000);              % do not take the entire sequence, just a section
    auxx = auxx/(max(abs(auxx)));
    if i == 1
        epoch_idx(i) = length(auxx);
    else
        epoch_idx(i) = epoch_idx(i-1) + length(auxx);
    end
    musict = [musict auxx];
end
l_auxx = length(auxx);
%epoch_idx

prevIIR = zeros(n,1);
epoch = 1;
tic
disp('Starting training epoch 0')
for i = M:length(musict)
    
    
    if i == epoch_idx(epoch)
        disp(['Finishing training epoch ' num2str(epoch)])
        epoch = epoch + 1;
    end
    
    % Filtering - Inner product
    musicin = musict(i-M+1:i);                       % M samples for online filtering
    filtpp = zeros(n,1);
    % Point-by-point Filtering
    for j = 1:n
        filtpp(j,1) = musicin*(fliplr(h(:,j)));
    end
    filtpp = filtpp.^2;
    Y = alph*filtpp + (1-alph)*prevIIR;              % Observation - square or absolute value
    prevIIR = Y;
    
    % First iteration - state estimation only
    if i == M   
        [XJ,P] = state_estimate(FJ,HJ,P,R,Q,zeros(k+n+s,1),Y);
        Z = XJ(1:s);
        U = XJ(s+1:s+k);
        X = XJ(s+k+1:s+k+n);
    else
        % State estimation
        Zp = XJ(1:s);
        Up = XJ(s+1:s+k);
        Xp = XJ(s+k+1:s+k+n);
        [XJ,P] = state_estimate(FJ,HJ,P,R,Q,XJ,Y);
        Z = XJ(1:s);
        U = XJ(s+1:s+k);
        X = XJ(s+k+1:s+k+n);

        % Parameter estimation for layer 2
        DG = [D,G];
        [DG,PDG] = parameter_estimate2(DG,[Zp;Up],U,r,PDG);
        D = DG(:,1:s);
        G = DG(:,s+1:s+k);

        % parameter estimation for layer 1
        BF = [B,F];
        [BF,PBF] = parameter_estimate2(BF,[Up;Xp],X,r,PBF);
        B = BF(:,1:k);
        F = BF(:,k+1:k+n);
        FJ = [C zeros(s,k) zeros(s,n); D G zeros(k,n) ; zeros(n,s) B,F];
    end        
end
toc

% Initial and Final Matrices after training
figure
% First Layer
subplot(2,4,1)
imagesc(B1)
title('Initial B matrix')
colorbar
subplot(2,4,5)
title('Final B matrix')
imagesc(B)
colorbar

subplot(2,4,2)
imagesc(F1)
title('Initial F matrix')
colorbar
subplot(2,4,6)
imagesc(F)
title('Final F matrix')
colorbar

% Second layer
subplot(2,4,3)
imagesc(D1)
title('Initial D matrix')
colorbar
subplot(2,4,7)
imagesc(D)
title('Final D matrix')
colorbar

subplot(2,4,4)
imagesc(G1)
title('Initial G matrix')
colorbar
subplot(2,4,8)
imagesc(G)
title('Final G matrix')
colorbar

save trumpet_training_params.mat FJ h 