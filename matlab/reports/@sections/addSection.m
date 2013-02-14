function ss = addSection(ss, varargin)
% function ss = addSection(ss, varargin)

% Copyright (C) 2013 Dynare Team
%
% This file is part of Dynare.
%
% Dynare is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% Dynare is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with Dynare.  If not, see <http://www.gnu.org/licenses/>.

assert(nargin >= 1 && nargin <= 3)
if nargin > 1
    assert(isa(varargin{1},'section'), ['Optional 2nd arg to addSection ' ...
                        'must be a Section']);
end

if nargin == 1
    ss.objArray = ss.objArray.addObj(section());
else
    ss.objArray = ss.objArray.addObj(varargin{:});
end
end