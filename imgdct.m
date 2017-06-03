function imgdct(name)
tic
slika=imread(name);
imshow(slika);

R=slika(:,:,1); 
G=slika(:,:,2); 
B=slika(:,:,3); 

%RGB ---> YCbCr
Y=    + 0.299*R + 0.587*G + 0.114*B;
U=128 - 0.1687*R - 0.3313*G + 0.5*B;
V=128 + 0.5*R - 0.4187*G - 0.0813*B;


[HH, WW]=size(R);
Hy=HH/8;
Wy=WW/8;

for y=0:Hy-1
   for x=0:Wy-1
      Bm=compress_block(Y, x*8, y*8, 1);     
      for b=1:8
           for a=1:8
              Y(y*8+b, x*8+a)=Bm(b, a);             
           end
      end    
     
      Bm=compress_block(U, x*8, y*8, 0);     
      for b=1:8
           for a=1:8
              U(y*8+b, x*8+a)=Bm(b, a);               
           end
      end
      
      Bm=compress_block(V, x*8, y*8, 0);    
      for b=1:8
           for a=1:8
              V(y*8+b, x*8+a)=Bm(b, a);            
           end
      end
   end
end



Y=double(Y);
U=double(U);
V=double(V);
R=double(R);
G=double(G);
B=double(B);
% %YCbCr ---> RGB
R= Y                  + 1.402*(V-128);
G= Y - 0.3441*(U-128) - 0.7141*(V-128);
B= Y + 1.772*(U-128);


for a=1:(Hy*8)
    for b=1:(Wy*8)
        slika(a, b, 1)=R(a, b);
        slika(a, b, 2)=G(a, b);
        slika(a, b, 3)=B(a, b);
    end
end

toc
hold;
figure;
imshow(slika, []);
end


function [r]=compress_block(Y, x, y, lum)
    r=zeros(8:8);
    for i=1:8
        for j=1:8
            r(i, j)=Y(y+i, x+j);           
        end
    end
    
    r=r-.128;
    r=dct2(r);
    
    if lum==1
        Q=[16, 11, 10, 16, 24, 40, 51, 61;
           12, 12, 14, 19, 26, 58, 60, 55;
           14, 13, 16, 24, 40, 57, 69, 56;
           14, 17, 22, 29, 51, 87, 80, 62;
           18, 22, 37, 56, 68, 109, 103, 77;
           24, 35, 55, 64, 81, 104, 113, 92;
           49, 64, 78, 87, 103, 121, 120, 101;
           72, 92, 95, 98, 112, 100, 103, 99];
       
        c=5;
    else
        Q=[17, 18, 24, 47, 99, 99, 99, 99;
           18, 21, 26, 66, 99, 99, 99, 99
           24, 26, 56, 99, 99, 99, 99, 99
           47, 66, 99, 99, 99, 99, 99, 99
           99, 99, 99, 99, 99, 99, 99, 99
           99, 99, 99, 99, 99, 99, 99, 99
           99, 99, 99, 99, 99, 99, 99, 99
           99, 99, 99, 99, 99, 99, 99, 99];  
       
        c=10;
    end
    
    r=r./Q;
    
    
    r=r./c;
    r=round(r);
    r=r.*Q;
    r=r.*c;

	r=idct2(r);
    r=r+.128;
	r=round(r);

end

