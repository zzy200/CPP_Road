#!/usr/bin/perl

use strict;
use warnings;
use CGI;
use CGI::Carp qw(fatalsToBrowser);

# 创建 CGI 对象
my $cgi = CGI->new;

# 获取参数
my $num1 = $cgi->param('num1') || 0;
my $num2 = $cgi->param('num2') || 0;
my $operation = $cgi->param('operation') || 'add';

# 验证输入
my $error;
if ($num1 !~ /^-?\d*\.?\d+$/) {
    $error = "Invalid number 1: $num1";
} elsif ($num2 !~ /^-?\d*\.?\d+$/) {
    $error = "Invalid number 2: $num2";
} elsif ($operation !~ /^(add|multiply)$/) {
    $error = "Invalid operation: $operation";
}

# 输出 HTTP 响应头
print "HTTP/1.1 200 OK\r\n";
print "Content-Type: text/html\r\n";
print "Connection: close\r\n";
print "\r\n";

# 输出 HTML
print <<HTML;
<html>
<head>
    <title>Calculator Result</title>
    <style>
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background-color: #f5f5f5;
            margin: 0;
            padding: 20px;
            color: #333;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 0 20px rgba(0, 0, 0, 0.1);
        }
        h1 {
            color: #2c3e50;
            text-align: center;
            margin-bottom: 30px;
        }
        .result {
            text-align: center;
            font-size: 24px;
            margin: 30px 0;
            padding: 20px;
            background-color: #f9f9f9;
            border-radius: 5px;
        }
        .error {
            color: #e74c3c;
            text-align: center;
            padding: 15px;
            background-color: #fdecea;
            border-radius: 5px;
            margin: 20px 0;
        }
        .back-btn {
            display: block;
            width: 100%;
            padding: 12px;
            text-align: center;
            background-color: #3498db;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            font-weight: 600;
            transition: background-color 0.3s;
        }
        .back-btn:hover {
            background-color: #2980b9;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Calculator Result</h1>
HTML

# 显示结果或错误
if ($error) {
    print "<div class='error'>Error: $error</div>";
} else {
    my $result;
    if ($operation eq 'add') {
        $result = $num1 + $num2;
    } elsif ($operation eq 'multiply') {
        $result = $num1 * $num2;
    }
    
    my $symbol = ($operation eq 'add') ? '+' : '*';
    print <<HTML;
        <div class="result">
            $num1 $symbol $num2 = <strong>$result</strong>
        </div>
HTML
}

# 添加返回链接
print <<HTML;
        <a href="/calculator.html" class="back-btn">Back to Calculator</a>
    </div>
</body>
</html>
HTML
