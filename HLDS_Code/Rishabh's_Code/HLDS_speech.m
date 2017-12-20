% clearvars

Fs = 44100;
tr1 = importdata('phonics.mat');
% [y1, Fs] = audioread('UI_MIS\Trumpet.novib.ff.C4B4.aiff');
y1 = tr1(1:397692);
y2 = tr1(397693:662178);
y3 = tr1(662179:927048);
y4 = tr1(927049:1191919);
y5 = tr1(1191920:1412645);
y6 = tr1(1412646:1633370);
y7 = tr1(1633371:1876168);
y8 = tr1(1876169:2141039);
y9 = tr1(2141040:2427983);
% [y122, Fs] = audioread('UI_MIS\Trumpet.novib.pp.E3B3.aiff');
% y = vertcat(y1,y2,y3,y4,y5,y6,y7,y8,y9,10,y11,y12);
% st1 = zeros(10,12);
% st2 = zeros(10,12);
% st3 = zeros(10,12);
% err = zeros(10,12);
% for u1 = 7:12
% yy=[y1;y2;y3;y4;y5;y6;y7;y8;y9];
y11 = downsample(y1, 4);
w = hann(882);
Fs1 = Fs/4;
G = zeros(882,floor(size(y11,1)/441)-1);
for i =1:floor(size(y11,1)/441)-1
    G(:,i) = y11((441*(i-1)+1):(441*(i+1)));
end
Z = zeros(64,size(G,2));
for i = 1:size(G,2)
    G1 = w.*G(:,i);
    L = size(G1,1);
    NFFT = 128;
    Y = fft(G1,NFFT)/L;
    f = Fs1/2*linspace(0,1,NFFT/2);
    Z(:,i) = 2*abs(Y(1:NFFT/2));
end

%State dimensions:
n = 60;
k = 10;
s = 3;
m = NFFT/2;

%Gammatone filter prep:
nfft = n;
sr = 44100;
nfilts = m;
width = 0.5;
minfreq = 10;
maxfreq = sr/2;
maxlen = nfft;

[H1,gain] = fft2gammatonemx(nfft, sr, nfilts, width, minfreq, maxfreq, maxlen);
H = [zeros(m,s) zeros(m,k) H1];

V=Z;
% for j = 1:10
%Coefficient matrices:
% Wt = 0.01.*eye(n+k+s);
pt = 0.01.*eye(s);
rt = 0.01.*eye(k);
wt = 0.01.*eye(n);

Wt = 0.01.*eye(n+k+s);

vt = 0.5.*eye(m);
I = eye(s);
G = rand(k);
D = rand(k, s);
F1 = rand(n);
B = rand(n, k);
% H1 = rand(m, n);

F = [I zeros(s,k) zeros(s,n); D G zeros(k,n); zeros(n,s) B F1];

%States Initialization:
zt = zeros(s, 1);
ut = zeros(k,1);
xt = zeros(n, 1);

Xt = [zt; ut; xt];

P = Wt;
ww = chol(Wt);

%---------------------------------


%Kalman Filter Implementation
l = 0.1;
% gg = ['r', 'g', 'b', 'k'];
gg = 'r';
% for j =1:2
%
%     if(j==2)
%         V = Z1;
%         gg = 'b';
%     else
%         V = Z;
%         gg = 'r';
%     end

st1 = zeros(100,1);
st2 = zeros(100,1);
st3 = zeros(100,1);
err = zeros(100,1);
% jj = zeros(m, size(V,2));

for j = 1:100
    
    Res1 = 0;
    Res2 = 0;
    Res3 = 0;
    er = 0;
    for i = 1:size(V,2)
        %STATE ESTImATE
        %Time Update
        Xt = F*Xt + randn(n+k+s,1);
        
        P = F*P*F' + Wt;
        
        %Measurement Update
        
        K = P*H'*inv(H*P*H' + vt);
        
        %     k1 = i + size(Z,2)*(j-1);
        %     if (norm(Z(:,i))~=0)
        %         jj(i) = norm(V(:,i)).*exp(-((norm(V(:,i)./norm(V(:,i))) - norm((H*Xt)./norm(H*Xt))).^2)/2*l^2).*norm(H*Xt);
        %     else
        %         jj(:,i) = 0;
        %     end
        %     Xt = Xt + K*sqrt((1/(sqrt(2*pi)*l)).*exp(-((V(:,i) - H*Xt).^2)/(2*(l^2))));
        %     Xt = Xt + K*(V(:,i) - H*Xt);
        Xt = Xt + exp(-(norm(V(:,i) - H*Xt)^2)/(2*(l^2)))*H'*(V(:,i) - H*Xt);
        
        P = (eye(s+k+n) - K*H)*P;
        
        %PARAMETERS ESTIMATION
        %Time Update
        
        
        %     Res1(i) = norm(Xt(1,:));
        %     Res2(i) = norm(Xt(2,:));
        %     Res3(i) = norm(Xt(3,:));
        % %     er = er + norm(V(:,i) - H*Xt);
        %     scatter3(Res1(i), Res2(i), Res3(i), gg)
        %     drawnow
        %     hold on
        Res1 = Res1 + Xt(1);
        Res2 = Res2 + Xt(2);
        Res3 = Res3 + Xt(3);
        er = er + norm(V(:,i) - H*Xt);
        %     figure(2)
        %     scatter3(Res1(i), Res2(i), Res3(i), gg)
        %     hold on
    end
    st1(j) = Res1/size(Z,2);
    st2(j) = Res2/size(Z,2);
    st3(j) = Res3/size(Z,2);
    err(j) = er/size(Z,2);
    
    % [mn, idx] = min(err(:,u1));
    figure(2)
    scatter3(st1(j), st2(j), st3(j), gg);
    drawnow
    hold on
end
