function [J, grad] = linearRegCostFunction(X, y, theta, lambda)

m = length(y); % ѵ��������Ŀ

% ��������Ҫ�������
J = 0; % �����������ȷ����ڴ�
grad = zeros(size(theta));

% X��һ��12 * 2�ľ���,��theta��һ��2*1���͵ľ���.
h = X * theta; % �õ��Ľ��,�ǰ�.

J = (1 / (2 * m)) * sum((h - y).^2) + (lambda/(2 * m)) *(sum(theta.^2) - theta(1)^2);

% J�Ѿ����������,Ȼ����Ҫ�ɵ�������Ǽ����ݶ���.
grad = (X' * (h - y))/ m + (lambda / m) * theta; % Ȼ��Ϳ�ʼ��.
grad(1) = grad(1) - lambda / m * theta(1);

grad = grad(:); % չ��

end
