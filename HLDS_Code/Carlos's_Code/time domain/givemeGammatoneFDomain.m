function [H,h]=givemeGammatoneFDomain(fs,fn,fft_s,M,op)
% H: one-sided frequency magnitude response matrix, size fft_s/2 x fn
% h: impulse response matrix, size window_size x fn
% fs: sampling frequency
% fn: number of gammatone filters
% fft_s: lenght of double-sided fft
% op: 1: full bandwidth option, 2: half bandwidth option

%M = 750;                         %%%%%% CHOICE
n = 4;                                      % filter order

% NOT USED
X1 = log(50)/log(10);
X2 = log(fs/2)/log(10);
f = logspace(X2,X1,fn);                         % Logarithmically spaced
                                                % center frequencies                                                    
b = 11.17268.*log(1+(46.06538.*f./(f+14678.49)));   % Equivalent rectangular 
                                                    % bandwith                                          
switch op
    case 1
        f(1) = 10;
        b(1) = 0.108*f(1)+24.7;
        for i=2:fn
            f(i)=f(i-1)+b(i-1);
            b(i)=0.108.*f(i)+24.7;
        end
    case 2                                      % Half bandwidth
        f(1) = 10;
        b(1) = 0.108*f(1)+24.7;
        for i=2:fn
            f(i)=f(i-1)+b(i-1)/2;
            b(i)=0.108.*f(i)+24.7;
        end
end
                                         
l = M;
tf = l/fs;                                  % length of window (in seconds)
t = 0:tf/(l-1):tf;                          % time instants

h = zeros(l,1*fn);
H = zeros(fft_s,1*fn);
for in = 1:fn    
    h(:,in)=(t.^(n-1)).*exp(-2*pi*b(in).*t).*cos(2*pi*f(in).*t);
    h(:,in)=h(:,in)./max(abs(1.*h(:,in)));              % normalization up to 1 amplitude
    H(:,in)=abs(fft(h(:,in),fft_s));
%     matrix(:,in)=matrix(:,in)./max(matrix(:,in));
end

H=H(1:fft_s/2,:);

end