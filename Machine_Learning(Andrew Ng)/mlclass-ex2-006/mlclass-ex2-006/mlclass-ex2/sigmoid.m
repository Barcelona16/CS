function g = sigmoid(z)
%SIGMOID Compute sigmoid functoon
%   J = SIGMOID(z) computes the sigmoid of z.

% You need to return the following variables correctly 
g = zeros(size(z)); % ����һ���µľ���
% matlabһ�����õ��������,û��ʲô���͵ĸ���,��֪���㴫���z�Ǹ�ʲô����
% ====================== YOUR CODE HERE ======================
% Instructions: Compute the sigmoid of each value of z (z can be a matrix,
%               vector or scalar).
% scalar��ʾ����
g = 1./(1 + exp(-z))
% =============================================================

end
