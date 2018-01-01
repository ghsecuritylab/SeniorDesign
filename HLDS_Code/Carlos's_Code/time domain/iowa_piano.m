clear
[A0, fs] = audioread('Piano.ff.A0.aiff'); 
[Bb2, fs] = audioread('Piano.ff.Bb2.aiff'); 
[Db3, fs] = audioread('Piano.ff.Db3.aiff'); 
[F3, fs] = audioread('Piano.ff.F3.aiff'); 
A0_d = decimate(A0(:,1),4); 
Bb2_d = decimate(Bb2(:,1),4); 
Db3_d = decimate(Db3(:,1),4); 
F3_d = decimate(F3(:,1),4); 


collection = {}; 
collection{1} = A0_d; 
collection{2} = Bb2_d; 
collection{3} = Db3_d; 
collection{4} = F3_d; 

save piano_iowa.mat collection