function A = extract(B,varargin)
% Extract some variables from a database.

A = dynSeries();

% Get the names of the variables to be extracted from dynSeries object B.
VariableName_ = {};
for i=1:nargin-1
    VariableName = varargin{i};
    idArobase = strfind(VariableName,'@');
    if length(idArobase)==2
        first_block_id = 0;
        last_block_id = 0;
        if idArobase(1)>1
            first_block_id = idArobase(1)-1;
        end
        if idArobase(2)<length(VariableName)
            last_block_id = length(VariableName)-idArobase(2)-1;
        end
        VariableName(idArobase(1)) = '[';
        VariableName(idArobase(2)) = ']';
        idVariables = find(isnotempty_cell(regexp(B.name,VariableName,'match')));
        if isempty(idVariables)
            error(['dynSeries::extract: Can''t find any variable matching ' VariableName ' pattern!'])
        end
        idVariables_ = [];
        for j = 1:length(idVariables)
            first_block_flag = 0;
            if (first_block_id && strcmp(B.name{idVariables(j)}(1:first_block_id),VariableName(1:first_block_id))) || ~first_block_id
                first_block_flag = 1;
            end
            last_block_flag = 0;
            if (last_block_id && strcmp(B.name{idVariables(j)}(end-last_block_id:end),VariableName(end-last_block_id:end))) || ~last_block_id
                last_block_flag = 1;
            end
            if first_block_flag && last_block_flag
                idVariables_ = [idVariables_; idVariables(j)];
            end
        end
        VariableName = B.name(idVariables_);
    end
    VariableName_ = vertcat(VariableName_,VariableName);
end

% Get indices of the selected variables
idVariableName = NaN(length(VariableName_),1);
for i = 1:length(idVariableName)
    idx = strmatch(VariableName_{i},B.name,'exact');
    if isempty(idx)
        error(['dynSeries::extract: Variable ' VariableName_{i} ' is not a member of ' inputname(1) '!'])
    end
    idVariableName(i) = idx;  
end

A.data = B.data(:,idVariableName);
A.init = B.init;
A.freq = B.freq;
A.nobs = B.nobs;
A.vobs = length(idVariableName);
A.name = B.name(idVariableName);




function b = isnotempty_cell(CellArray)
    CellArrayDimension = size(CellArray);
    b = NaN(CellArrayDimension);
    for i=1:CellArrayDimension(1)
        for j = 1:CellArrayDimension(2)
            b(i,j) = ~isempty(CellArray{i,j});
        end
    end