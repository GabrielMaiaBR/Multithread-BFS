filename = 'bfs_comparison_results.csv';
data = readtable(filename);

vertices = data.Vertices;
bfs_without_threads = data.BFS_without_threads;
bfs_with_threads = data.BFS_with_threads;

figure;
hold on;

plot(vertices, bfs_without_threads, '-o', 'DisplayName', 'BFS without threads');

plot(vertices, bfs_with_threads, '-x', 'DisplayName', 'BFS with threads');

xlabel('# of vertex');
ylabel('Time (microseconds)');
title('BFS Performance Comparison');
legend('show');
grid on;
hold off;