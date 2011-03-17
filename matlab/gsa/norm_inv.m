function invp = norm_inv(x, m, sd)
% PURPOSE: computes the quantile (inverse of the CDF) 
%          for each component of x with mean m, standard deviation sd
%---------------------------------------------------
% USAGE: invp = norm_inv(x,m,v)
% where: x = variable vector (nx1)
%        m = mean vector (default=0)
%        sd = standard deviation  vector (default=1)
%---------------------------------------------------
% RETURNS: invp (nx1) vector
%---------------------------------------------------
% SEE ALSO: norm_d, norm_rnd, norm_inv, norm_cdf
%---------------------------------------------------

% Written by KH (Kurt.Hornik@ci.tuwien.ac.at) on Oct 26, 1994
% Copyright Dept of Probability Theory and Statistics TU Wien
% Converted to MATLAB by JP LeSage, jpl@jpl.econ.utoledo.edu

  if nargin > 3
    error ('Wrong # of arguments to norm_inv');
  end

  [r, c] = size (x);
  s = r * c;
  
  if (nargin == 1)
    m = zeros(1,s);
    sd = ones(1,s);
  end


 if length(m)==1,
   m = repmat(m,1,s);
 end
 if length(sd)==1,
   sd = repmat(sd,1,s);
 end
  x = reshape(x,1,s);
  m = reshape(m,1,s);
  sd = reshape(sd,1,s);

  invp = zeros (1,s);

    invp = m + sd .* (sqrt(2) * erfinv(2 * x - 1)); 
    
     
  
    invp = reshape (invp, r, c);
  