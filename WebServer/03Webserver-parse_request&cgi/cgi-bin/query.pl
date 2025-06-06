#!/usr/bin/perl

use strict;
use warnings;
use DBI;         # 数据库接口
use CGI;         # CGI模块
use CGI::Carp qw(fatalsToBrowser);  # 将错误显示到浏览器

# 创建CGI对象
my $cgi = CGI->new;

# 数据库配置 - 替换为你的实际凭据
my $database = "student_db";
my $host = "localhost";
my $user = "webuser";
my $password = "zzy040515";

# 获取学生ID
my $student_id = $cgi->param('student_id') || '';

# 输出HTTP响应头
print "HTTP/1.1 200 OK\r\n";
print "Content-Type: text/html\r\n";
print "Connection: close\r\n";
print "\r\n";

# 输出HTML头部
print <<HTML;
<html>
<head>
    <title>Student Query Result</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #f5f5f5;
            margin: 0;
            padding: 20px;
            color: #333;
        }
        .container {
            max-width: 800px;
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
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 12px;
            text-align: left;
        }
        th {
            background-color: #3498db;
            color: white;
        }
        tr:nth-child(even) {
            background-color: #f2f2f2;
        }
        .error {
            color: #e74c3c;
            padding: 15px;
            background-color: #fdecea;
            border-radius: 5px;
            margin: 20px 0;
        }
        .back-btn {
            display: inline-block;
            padding: 10px 20px;
            background-color: #3498db;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            font-weight: 600;
            transition: background-color 0.3s;
            margin-top: 20px;
        }
        .back-btn:hover {
            background-color: #2980b9;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Student Information</h1>
HTML

# 验证输入
unless ($student_id =~ /^\d{4}$/) {
    print "<div class='error'>Error: Invalid Student ID format. Must be 4 digits.</div>";
    print "<a href='/query.html' class='back-btn'>Back to Query</a>";
    print "</div></body></html>";
    exit;
}

# 连接到数据库
my $dsn = "DBI:mysql:database=$database;host=$host";
my $dbh = DBI->connect($dsn, $user, $password, {
    RaiseError => 0,  # 禁用异常抛出
    PrintError => 0   # 禁用自动错误打印
});

# 检查连接是否成功
unless ($dbh) {
    my $error = DBI::errstr || "Unknown database error";
    print "<div class='error'>Database connection failed: $error</div>";
    print "<a href='/query.html' class='back-btn'>Back to Query</a>";
    print "</div></body></html>";
    exit;
}

# 准备查询（使用参数化查询防止SQL注入）
my $query = "SELECT id, name, class FROM students WHERE id = ?";
my $sth = $dbh->prepare($query);

# 检查准备是否成功
unless ($sth) {
    my $error = $dbh->errstr || "Unknown query preparation error";
    print "<div class='error'>Query preparation failed: $error</div>";
    $dbh->disconnect;
    print "<a href='/query.html' class='back-btn'>Back to Query</a>";
    print "</div></body></html>";
    exit;
}

# 执行查询
my $success = $sth->execute($student_id);

# 检查执行是否成功
unless ($success) {
    my $error = $sth->errstr || "Unknown query execution error";
    print "<div class='error'>Query execution failed: $error</div>";
    $sth->finish;
    $dbh->disconnect;
    print "<a href='/query.html' class='back-btn'>Back to Query</a>";
    print "</div></body></html>";
    exit;
}

# 获取结果
my $result = $sth->fetchrow_hashref;

# 显示结果
if ($result) {
    print <<HTML;
    <table>
        <tr>
            <th>Student ID</th>
            <th>Name</th>
            <th>Class</th>
        </tr>
        <tr>
            <td>$result->{id}</td>
            <td>$result->{name}</td>
            <td>$result->{class}</td>
        </tr>
    </table>
HTML
} else {
    print "<p>No student found with ID: $student_id</p>";
}

# 添加返回链接
print "<a href='/query.html' class='back-btn'>Back to Query</a>";

# 清理资源
$sth->finish;
$dbh->disconnect;

# 输出HTML尾部
print <<HTML;
    </div>
</body>
</html>
HTML
