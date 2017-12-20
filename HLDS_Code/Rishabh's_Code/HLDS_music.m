clearvars

Fs = 44100;
[y{1}, Fs] = audioread('UI_MIS\Trumpet.novib.ff.C4B4.aiff');
% [y2, Fs] = audioread('UI_MIS\Trumpet.novib.ff.C5B5.aiff');
% [y3, Fs] = audioread('UI_MIS\Trumpet.novib.ff.C6Eb6.aiff');
% [y4, Fs] = audioread('UI_MIS\Trumpet.novib.ff.E3B3.aiff');
% [y5, Fs] = audioread('UI_MIS\Trumpet.novib.mf.C4B4.aiff');
% [y6, Fs] = audioread('UI_MIS\Trumpet.novib.mf.C5B5.aiff');
% [y7, Fs] = audioread('UI_MIS\Trumpet.novib.mf.C6D6.aiff');
[y{2}, Fs] = audioread('UI_MIS\Trumpet.novib.mf.E3B3.aiff');
% [y9, Fs] = audioread('UI_MIS\Trumpet.novib.pp.C4B4.aiff');
% [y10, Fs] = audioread('UI_MIS\Trumpet.novib.pp.C5B5.aiff');
[y{3}, Fs] = audioread('UI_MIS\Trumpet.novib.pp.C6Eb6.aiff');
% [y122, Fs] = audioread('UI_MIS\Trumpet.novib.pp.E3B3.aiff');
% y = vertcat(y1,y2,y3,y4,y5,y6,y7,y8,y9,10,y11,y12);
st1 = zeros(10,3);
st2 = zeros(10,3);
st3 = zeros(10,3);
err = zeros(10,3);
for u1 = 1:3
    y11 = downsample(y{u1}, 4);
    w = hann(882);
    Fs1 = Fs/4;
    G = zeros(882,floor(size(y11,1)/441)-1);
    for i =1:floor(size(y11,1)/441)-1
        G(:,i) = y11((441*(i-1)+1):(441*(i+1)));
    end
    Z = zeros(512,size(G,2));
    for i = 1:size(G,2)
        G1 = w.*G(:,i);
        L = size(G1,1);
        NFFT = 1024;
        Y = fft(G1,NFFT)/L;
        f = Fs1/2*linspace(0,1,NFFT/2);
        Z(:,i) = 2*abs(Y(1:NFFT/2));
    end
    
    %State dimensions:
    n = 60;
    k = 10;
    s = 3;
    m = 512;
    
    %Gammatone filter prep:
    nfft = n;
    sr = 44100;
    nfilts = 512;
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
    
    P = Wt;        % why? 
    ww = chol(Wt); % why? 
    
    %---------------------------------
    
    %Kalman Filter Implementation
    l = 0.9;
    gg = ['r', 'g', 'b', 'k'];
    % for j =1:2
    %
    %     if(j==2)
    %         V = Z1;
    %         gg = 'b';
    %     else
    %         V = Z;
    %         gg = 'r';
    %     end
    
    for j = 1:10
        
        Res1 = 0;
        Res2 = 0;
        Res3 = 0;
        er = 0;
        for i = 1:size(V, 2)
            %STATE ESTImATE
            %Time Update
            Xt = F*Xt + randn(n+k+s,1);
            
            P = F*P*F' + Wt;
            
            %Measurement Update
            
            K = P*H'*inv(H*P*H' + vt);
            
            %     k1 = i + size(Z,2)*(j-1);
            %     if (norm(Z(:,i))~=0)
            jj(i) = norm(V(:,i))*exp(-((norm((V(:,i)./norm(V(:,i))) - ((H*Xt)./norm(H*Xt))))^2)/2*l^2)*norm(H*Xt);
            %     else
            %         jj(i) = 0;
            %     end
            Xt = Xt + K*(V(:,i) - H*Xt + jj(i));
            
            P = (eye(s+k+n) - K*H)*P;
            
            %PARAMETERS ESTIMATION
            %Time Update
            
            
            Res1 = Res1 + norm(Xt(1,:));
            Res2 = Res2 + norm(Xt(2,:));
            Res3 = Res3 + norm(Xt(3,:));
            er = er + norm(V(:,i) - H*Xt);
            %     figure(2)
            %     scatter3(Res1(i), Res2(i), Res3(i), gg)
            %     hold on
        end
        st1(j,u1) = Res1/size(Z,2);
        st2(j,u1) = Res2/size(Z,2);
        st3(j,u1) = Res3/size(Z,2);
        err(j,u1) = er/size(Z,2);
        
        % [mn, idx] = min(err(:,u1));
        figure(2)
        scatter3(st1(j,u1), st2(j,u1), st3(j,u1), gg(u1));
        hold on
    end
end
hold off
