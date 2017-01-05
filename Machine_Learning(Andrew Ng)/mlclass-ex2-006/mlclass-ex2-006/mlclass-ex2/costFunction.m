function [J, grad] = costFunction(theta, X, y)
%COSTFUNCTION Compute cost and gradient for logistic regression
%   J = COSTFUNCTION(theta, X, y) computes the cost of using theta as the
%   parameter for logistic regression and the gradient of the cost
%   w.r.t. to the parameters.

% Initialize some useful values
m = length(y); % ѵ��������Ŀ

% You need to return the following variables correctly 
J = 0; % ����һ������ϵ��
grad = zeros(size(theta)); % ��Ӧ�������յ�һ���������,�ǰ�.

% ====================== YOUR CODE HERE ======================
% Instructions: Compute the cost of a particular choice of theta.
%               You should set J to the cost.
%               Compute the partial derivatives and set grad to the partial
%               derivatives of the cost w.r.t. each parameter in theta
%
% Note: grad should have the same dimensions as theta
%
% �����Ѿ�ӵ�е�һЩ����:X��ѵ����,Ȼ��y��label,���ǻ��г�ʼ��theta
% ����ܹ����������а�,�ֵ�

% thetaӦ����һ��

h = sigmoid(X * theta)
J = (1 / m) * sum(-y.*log(h) - (1 - y).*log(1 - h))
% ������Ҫ�����ݶ���
% �����ȼ���������,���������j=0�������
grad = (1/m) * (X' *(h - y))




% =============================================================

end
