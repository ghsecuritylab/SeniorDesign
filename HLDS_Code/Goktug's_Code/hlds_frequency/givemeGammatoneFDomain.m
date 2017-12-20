function [matrix]=givemeGammatoneFDomain(fs,filter_number,fft_size,erbSpacing)

if nargin < 4
    erbSpacing = 2;
end

window_size=1000;
% window_size=500;
n=4;                %filter order

% X1=log(50)/log(10);
% X2=log(fs/2)/log(10);

% f=logspace(X2,X1,filter_number);                    %Logarithmically spaced
                                                    %center frequencies
                                                    
% b=11.17268.*log(1+(46.06538.*f./(f+14678.49)));   %Equivalent Rectangular 
                                                    %Bandwith
% f(1)=10;
% b(1)=0.108*f(1)+24.7;
% for i=2:filter_number
%     f(i)=f(i-1)+b(i-1);
%     b(i)=0.108.*f(i)+24.7;
% end

f(1)=10; %trial change half bandwith
b(1)=0.108*f(1)+24.7;
for i=2:filter_number
    f(i)=f(i-1)+b(i-1)/erbSpacing;
    b(i)=0.108.*f(i)+24.7;
end
                                                    
l=window_size;
tf=l/fs;        %length of window (in seconds)
t = linspace(0,tf,l);
% t=0:tf/(l-1):tf;   %time instants

C=zeros(l,1*filter_number);
matrix=zeros(fft_size,1*filter_number);
for in=1:filter_number    
    C(:,in)=(t.^(n-1)).*exp(-2*pi*b(in).*t).*cos(2*pi*f(in).*t);
    C(:,in)=C(:,in)./max(abs(2.*C(:,in)));
    matrix(:,in)=abs(fft(C(:,in),fft_size));
%     matrix(:,in)=matrix(:,in)./max(matrix(:,in));
end

matrix=matrix(1:fft_size/2,:);

end