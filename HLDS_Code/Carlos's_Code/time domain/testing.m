clear 
load trumpet_training_params.mat 

% HDLS parameters
Fs = 11025;                       % sampling frequency
s = 3;                          % dimension of the third layer states (z)
k = 10;                         % dimension of the second layer states (u)
n = 60;                         % dimension of the first layer states (x) - also number of filters for this case
m = n;                          % dimension  of the observation (y)
alph = 0.005;

% Noise parameters
r = 0.1;                        % covariance parameter for parameter/matrices estimation
Q = 0.01*eye(k+n+s);             % process noise covariance matrix
R = 0.5;                        % measurement noise variance
P = eye(k+n+s);                 % Initialization of covariance matrix for joint state

% Number of data points for filtering 
M = 1000;

% Observation-layer for time domain approach
H = eye(m);

% Joint measurement matrix
HJ = [zeros(m,s) zeros(m,k) H]; 

% Joint state transition matrix
% FJ 

% Initial state 
XJ = zeros(k+n+s,1); 

% Load and format trumpet notes 
load('trumpet_iowa_cell.mat')
col =  collection(:,1:35);
num_of_notes = 8; 
note_length = 3000; 

musict = zeros(0,0);

freq_index = [3 6 35 20 24]; 
for i = 1:length(freq_index)
%     music_sel = randi([1 35]);
    auxx = (col{freq_index(i)})';
%     auxx = (col{music_sel})';
    aux_idx = round(length(auxx)/2);
    auxx = auxx(aux_idx-note_length/2:aux_idx+note_length/2-1);              % do not take the entire sequence, just a section
    auxx = auxx/(max(abs(auxx)));

    musict = [musict auxx];
end
l_auxx = length(auxx);

% Init previous IIR output 
prevIIR = zeros(n,1);

%figure; 
u1 = 1; 
gg = ['r', 'g', 'b', 'k', 'm', 'c', 'y', 'r', 'g', 'b', 'k', 'm'];

colorCnt = M;
plotCnt = 1; % granularity for points plotted 
for i = M:length(musict)
    musicin = musict(i-M+1:i);

    % Filtering - Inner product
    filtpp = zeros(n,1);
    % Point-by-point Filtering
    for j = 1:n
        filtpp(j,1) = musicin*(fliplr(h(:,j)));
    end
    filtpp = filtpp.^2;
    Y = alph*filtpp + (1-alph)*prevIIR;              % Observation - square or absolute value
    prevIIR = Y;

    % State estimation 
    [XJ,P] = state_estimate(FJ,HJ,P,R,Q,XJ,Y);
    Z = XJ(1:s);
    U = XJ(s+1:s+k);
    X = XJ(s+k+1:s+k+n);

    plotCnt = plotCnt + 1; 
    if (plotCnt > 25) 
       scatter3(Z(1), Z(2), Z(3), gg(u1)); hold on; 
       plotCnt = 1; 
   end 

    colorCnt = colorCnt + 1; 
    if (colorCnt == note_length) 
        u1 = u1+1;  
        colorCnt = 0; 
    end 
end 
hold off; 