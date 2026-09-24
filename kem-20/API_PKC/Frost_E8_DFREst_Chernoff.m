function results = Frost_E8_DFREst_Chernoff_CC(whichPreset)
% Frost_E8_DFREst_Chernoff_CC.m
%
% Certified Chernoff upper-bound DFR estimator for the optional E8
% lattice-code embedding in Frost.
%
% An E8 block is either one ciphertext column (ell_r = 8) or one ciphertext row (ell_s = 8).
% Let alpha_E8 = q / 2^b_msg.
% A block decodes correctly when the normalized error vector e / alpha_E8
% remains in the Voronoi cell of E8.
%
% For E8, the Voronoi cell is described by the 240 roots r of E8.
% With the standard normalization in which roots have squared norm 2,
%
%   e is inside alpha_E8 * Vor(E8)  iff  |<e,r>| < alpha_E8 for all roots r.
%
% Hence this script computes the upper bound
%
%   P_block <= sum_{r in Roots(E8)} P[ |<E_blk,r>| >= alpha_E8 ].
%
% Each one-dimensional projection tail is upper bounded by Chernoff's method.
% For X = <E_blk,r>,
%
%   P[X >= t] <= inf_{theta > 0} exp(K_X(theta) - theta t),
%   P[X <= -t] <= inf_{theta < 0} exp(K_X(theta) + theta t),
%
% where K_X(theta) = log E exp(theta X).
% The CGF is evaluated exactly from the finite component PMFs.
% The only remaining numerical error is floating-point optimization error.
%
% Usage:
%   results = Frost_E8_DFREst_Chernoff_CC();
%   results = Frost_E8_DFREst_Chernoff_CC('Frost128');
%   results = Frost_E8_DFREst_Chernoff_CC('Frost384');
%   results = Frost_E8_DFREst_Chernoff_CC('Frost512');
%   results = Frost_E8_DFREst_Chernoff_CC('Frost384CC');
%   results = Frost_E8_DFREst_Chernoff_CC('Frost512CC');
%   results = Frost_E8_DFREst_Chernoff('all');
%
% Notes:
%   1. This is an upper bound under the stated independent component model.
%   2. The E8 block dimension is fixed to 8.
%      Default profiles use column-wise E8 blocks with ell_r = 8.
%      Compact-ciphertext profiles use row-wise E8 blocks with ell_s = 8.
%   3. Rectangular message matrices are supported.
%      There are ell_s column-wise blocks or ell_r row-wise blocks per ciphertext.
%   4. The b_vec field is validated as payload bookkeeping.
%      The DFR bound itself uses the scaled E8 Voronoi rule.
clc
clear
if nargin < 1 || isempty(whichPreset)
    whichPreset = 'all';
end

presets = preset_configs();
names = fieldnames(presets);

if isstring(whichPreset)
    whichPreset = char(whichPreset);
end

if strcmpi(whichPreset, 'all')
    selected = names;
else
    key = normalize_preset_name(whichPreset);
    if ~isfield(presets, key)
        error('Unknown preset %s. Use Frost128, Frost192, Frost256, Frost384, Frost512, or all.', whichPreset);
    end
    selected = {key};
end

results = struct();
for t = 1:numel(selected)
    key = selected{t};
    cfg = presets.(key);
    results.(key) = run_one_cfg(cfg);
end

end

% -------------------------------------------------------------------------
% Presets.
% -------------------------------------------------------------------------

function presets = preset_configs()
presets = struct();

presets.Frost128 = struct( ...
    'name', 'Frost-128', ...
    'n', 512, ...
    'm', 512, ...
    'ell_r', 8, ...
    'ell_s', 8, ...
    'b_msg', 2, ...
    'b_vec', [1 2 2 2 2 2 2 3], ...
    'q', 2^15, ...
    'p_pk', 2^10, ...
    'p_u', 2^10, ...
    'p_v', 2^5, ...
    'eta_s', 2, ...
    'eta_r', 2 ...
    );

presets.Frost192 = struct( ...
    'name', 'Frost-192', ...
    'n', 880, ...
    'm', 880, ...
    'ell_r', 8, ...
    'ell_s', 8, ...
    'b_msg', 3, ...
    'b_vec', [2 3 3 3 3 3 3 4], ...
    'q', 2^16, ...
    'p_pk', 2^11, ...
    'p_u', 2^11, ...
    'p_v', 2^6, ...
    'eta_s', 1, ...
    'eta_r', 1 ...
    );

presets.Frost256 = struct( ...
    'name', 'Frost-256', ...
    'n', 1288, ...
    'm', 1288, ...
    'ell_r', 8, ...
    'ell_s', 8, ...
    'b_msg', 4, ...
    'b_vec', [3 4 4 4 4 4 4 5], ...
    'q', 2^16, ...
    'p_pk', 2^13, ...
    'p_u', 2^12, ...
    'p_v', 2^8, ...
    'eta_s', 1, ...
    'eta_r', 1 ...
    );

% Default high-security profiles: column-wise E8 blocks.
presets.Frost384 = struct( ...
    'name', 'Frost-384', ...
    'n', 1928, ...
    'm', 1928, ...
    'ell_r', 8, ...
    'ell_s', 12, ...
    'b_msg', 4, ...
    'b_vec', [3 4 4 4 4 4 4 5], ...
    'q', 2^16, ...
    'p_pk', 2^13, ...
    'p_u', 2^13, ...
    'p_v', 2^9, ...
    'eta_s', 1, ...
    'eta_r', 1 ...
    );

presets.Frost512 = struct( ...
    'name', 'Frost-512', ...
    'n', 2600, ...
    'm', 2600, ...
    'ell_r', 8, ...
    'ell_s', 16, ...
    'b_msg', 4, ...
    'b_vec', [3 4 4 4 4 4 4 5], ...
    'q', 2^16, ...
    'p_pk', 2^14, ...
    'p_u', 2^14, ...
    'p_v', 2^7, ...
    'eta_s', 1, ...
    'eta_r', 1 ...
    );

% Compact-ciphertext variants: row-wise E8 blocks.
presets.Frost384CC = struct( ...
    'name', 'Frost-CC-384', ...
    'n', 1928, ...
    'm', 1928, ...
    'ell_r', 12, ...
    'ell_s', 8, ...
    'b_msg', 4, ...
    'b_vec', [3 4 4 4 4 4 4 5], ...
    'q', 2^16, ...
    'p_pk', 2^13, ...
    'p_u', 2^13, ...
    'p_v', 2^9, ...
    'eta_s', 1, ...
    'eta_r', 1 ...
    );

presets.Frost512CC = struct( ...
    'name', 'Frost-CC-512', ...
    'n', 2600, ...
    'm', 2600, ...
    'ell_r', 16, ...
    'ell_s', 8, ...
    'b_msg', 4, ...
    'b_vec', [3 4 4 4 4 4 4 5], ...
    'q', 2^16, ...
    'p_pk', 2^14, ...
    'p_u', 2^14, ...
    'p_v', 2^7, ...
    'eta_s', 1, ...
    'eta_r', 1 ...
    );
end

function key = normalize_preset_name(s)
s = lower(strrep(strrep(char(s), '-', ''), '_', ''));
s = strrep(s, '+', 'plus');
s = strrep(s, 'cc', 'cc');
switch s
    case {'frost128','128','l1'}
        key = 'Frost128';
    case {'frost192','192','l3'}
        key = 'Frost192';
    case {'frost256','256','l5'}
        key = 'Frost256';
    case {'frost384','384'}
        key = 'Frost384';
    case {'frost512','512'}
        key = 'Frost512';
    case {'frost384cc','frostcc384','384cc','cc384'}
        key = 'Frost384CC';
    case {'frost512cc','frostcc512','512cc','cc512'}
        key = 'Frost512CC';
    otherwise
        key = char(s);
end
end

% -------------------------------------------------------------------------
% One configuration.
% -------------------------------------------------------------------------

function result = run_one_cfg(cfg)
validate_cfg(cfg);

axis = e8_block_axis(cfg);
num_blocks = e8_block_count(cfg);

alpha_E8 = cfg.q / 2^cfg.b_msg;
scalar_threshold = alpha_E8 / 2;

scalar_stats = make_scalar_total_stats(cfg);
log2_scalar_single = log2_chernoff_two_sided(scalar_stats, scalar_threshold);
log2_scalar_whole = min(0, log2_scalar_single + log2(cfg.ell_r * cfg.ell_s));

[root_groups, root_threshold_scales] = e8_root_groups();
log2_terms = -inf(numel(root_groups), 1);
term_info = struct([]);

for i = 1:numel(root_groups)
    coeffs = root_groups(i).coeffs;
    multiplicity = root_groups(i).multiplicity;
    threshold = root_threshold_scales(i) * alpha_E8;

    proj_stats = make_projection_stats(cfg, coeffs, axis);
    log2_tail = log2_chernoff_two_sided(proj_stats, threshold);
    log2_terms(i) = log2_tail + log2(multiplicity);

    term_info(i).name = root_groups(i).name; %#ok<AGROW>
    term_info(i).multiplicity = multiplicity;
    term_info(i).coeffs = coeffs;
    term_info(i).threshold = threshold;
    term_info(i).log2_tail_chernoff = log2_tail;
    term_info(i).log2_weighted_tail_chernoff = log2_terms(i);
end

log2_block_union = min(0, log2_sum(log2_terms));
log2_whole_union = min(0, log2_block_union + log2(num_blocks));

result = struct();
result.cfg = cfg;
result.alpha_E8 = alpha_E8;
result.scalar_threshold = scalar_threshold;
result.log2_scalar_single_chernoff = log2_scalar_single;
result.log2_scalar_whole_chernoff_union = log2_scalar_whole;
result.log2_E8_block_chernoff_union = log2_block_union;
result.log2_E8_whole_ciphertext_chernoff_union = log2_whole_union;
result.root_terms = term_info;
result.e8_block_axis = axis;
result.e8_num_blocks = num_blocks;

fprintf('============================================================\n');
fprintf('Preset %s\n', cfg.name);
fprintf('n = %d, m = %d, ell_r = %d, ell_s = %d\n', cfg.n, cfg.m, cfg.ell_r, cfg.ell_s);
fprintf('q = %d, p_pk = %d, p_u = %d, p_v = %d\n', cfg.q, cfg.p_pk, cfg.p_u, cfg.p_v);
fprintf('eta_s = %d, eta_r = %d\n', cfg.eta_s, cfg.eta_r);
fprintf('b_msg = %d\n', cfg.b_msg);
fprintf('b_vec = [%s]\n', sprintf('%d ', cfg.b_vec));
fprintf('FO plaintext bits = %d\n', num_blocks * sum(cfg.b_vec));
fprintf('alpha_E8 = %.0f\n', alpha_E8);
fprintf('baseline scalar threshold = alpha_E8/2 = %.4f\n', scalar_threshold);
fprintf('baseline scalar single-coefficient Chernoff log2 upper bound = %.8f\n', log2_scalar_single);
fprintf('baseline whole-ciphertext scalar Chernoff union log2 upper bound = %.8f\n', log2_scalar_whole);
fprintf('\n');
fprintf('E8 Voronoi-block Chernoff upper bound\n');
fprintf('E8 block orientation = %s\n', axis);
fprintf('E8 blocks per ciphertext = %d\n', num_blocks);
fprintf('single-block E8 Voronoi-facet Chernoff union log2 upper bound = %.8f\n', log2_block_union);
fprintf('whole-ciphertext E8 Chernoff union log2 upper bound = %.8f\n', log2_whole_union);
fprintf('dominant root group = %s\n', dominant_term(term_info));
fprintf('============================================================\n');

end

function validate_cfg(cfg)
fields = {'name','n','m','ell_r','ell_s','b_msg','b_vec','q','p_pk','p_u','p_v','eta_s','eta_r'};
for i = 1:numel(fields)
    if ~isfield(cfg, fields{i})
        error('Missing cfg.%s', fields{i});
    end
end
if cfg.ell_r < 1 || floor(cfg.ell_r) ~= cfg.ell_r
    error('ell_r must be a positive integer.');
end
if cfg.ell_s < 1 || floor(cfg.ell_s) ~= cfg.ell_s
    error('ell_s must be a positive integer.');
end
if cfg.ell_r ~= 8 && cfg.ell_s ~= 8
    error('This E8 estimator requires either ell_r = 8 for column-wise decoding or ell_s = 8 for row-wise decoding.');
end
if numel(cfg.b_vec) ~= 8
    error('cfg.b_vec must have length 8.');
end
if sum(cfg.b_vec) ~= 8 * cfg.b_msg
    error('E8 payload mismatch: sum(b_vec) must equal 8*b_msg.');
end
if mod(cfg.q, cfg.p_pk) ~= 0 || mod(cfg.q, cfg.p_u) ~= 0 || mod(cfg.q, cfg.p_v) ~= 0
    error('This script assumes p_pk | q, p_u | q, and p_v | q.');
end
end

function axis = e8_block_axis(cfg)
if cfg.ell_r == 8
    axis = 'columns';
elseif cfg.ell_s == 8
    axis = 'rows';
else
    error('No valid E8 block axis.');
end
end

function num_blocks = e8_block_count(cfg)
axis = e8_block_axis(cfg);
switch axis
    case 'columns'
        num_blocks = cfg.ell_s;
    case 'rows'
        num_blocks = cfg.ell_r;
    otherwise
        error('Unknown E8 block axis.');
end
end

% -------------------------------------------------------------------------
% E8 roots grouped by symmetry.
% -------------------------------------------------------------------------

function [groups, threshold_scales] = e8_root_groups()
% Type 1 roots: permutations of (±1, ±1, 0^6), 112 roots.
% Type 2 roots: (±1/2,...,±1/2) with an even number of minus signs, 128 roots.
% For Type 2, doubled coefficients ±1 are used and the threshold is 2*alpha_E8.

groups = struct([]);
threshold_scales = [];

pair_coeffs = {[1 1], [1 -1], [-1 1], [-1 -1]};
pair_names = {'pair++', 'pair+-', 'pair-+', 'pair--'};
for i = 1:4
    groups(end+1).name = pair_names{i}; %#ok<AGROW>
    groups(end).coeffs = pair_coeffs{i};
    groups(end).multiplicity = nchoosek(8, 2);
    threshold_scales(end+1) = 1; %#ok<AGROW>
end

for kminus = [0 2 4 6 8]
    groups(end+1).name = sprintf('half-root kminus=%d', kminus); %#ok<AGROW>
    groups(end).coeffs = [ones(1, 8-kminus), -ones(1, kminus)];
    groups(end).multiplicity = nchoosek(8, kminus);
    threshold_scales(end+1) = 2; %#ok<AGROW>
end
end

function name = dominant_term(term_info)
vals = [term_info.log2_weighted_tail_chernoff];
[~, idx] = max(vals);
name = sprintf('%s (weighted log2 %.8f)', term_info(idx).name, term_info(idx).log2_weighted_tail_chernoff);
end

% -------------------------------------------------------------------------
% Projection distributions through exact finite-support CGFs.
% -------------------------------------------------------------------------

function stats = make_scalar_total_stats(cfg)
% One scalar coefficient: E = sum_m Epk*R - sum_n S*Eu + Ev.
pmf_s  = centered_binomial_pmf(cfg.eta_s);
pmf_r  = centered_binomial_pmf(cfg.eta_r);
pmf_pk = divisible_quant_error_pmf(cfg.q, cfg.p_pk);
pmf_u  = divisible_quant_error_pmf(cfg.q, cfg.p_u);
pmf_v  = divisible_quant_error_pmf(cfg.q, cfg.p_v);

pmf_pk_times_r = product_pmf(pmf_pk, pmf_r);
pmf_s_times_u  = product_pmf(pmf_s,  pmf_u);

components = struct([]);
components(1).count = cfg.m;
components(1).supp = pmf_pk_times_r.supp;
components(1).prob = pmf_pk_times_r.prob;
components(2).count = cfg.n;
components(2).supp = -pmf_s_times_u.supp;
components(2).prob = pmf_s_times_u.prob;
components(3).count = 1;
components(3).supp = pmf_v.supp;
components(3).prob = pmf_v.prob;

stats = make_cgf_stats(components);
end

function stats = make_projection_stats(cfg, coeffs, axis)
if nargin < 3 || isempty(axis)
    axis = e8_block_axis(cfg);
end
switch axis
    case 'columns'
        stats = make_projection_stats_columnwise(cfg, coeffs);
    case 'rows'
        stats = make_projection_stats_rowwise(cfg, coeffs);
    otherwise
        error('Unknown E8 block axis %s.', axis);
end
end

function stats = make_projection_stats_columnwise(cfg, coeffs)
% Column-wise projection <E(:,j), coeffs>.
% Used by the default profiles with ell_r = 8.
% For E = Epk^T R - S^T Eu + Ev, this gives
%   sum_a <Epk(a,:),coeffs> R(a,j)
% - sum_b <S(b,:),coeffs> Eu(b,j)
% + <Ev(:,j),coeffs>.
pmf_s  = centered_binomial_pmf(cfg.eta_s);
pmf_r  = centered_binomial_pmf(cfg.eta_r);
pmf_pk = divisible_quant_error_pmf(cfg.q, cfg.p_pk);
pmf_u  = divisible_quant_error_pmf(cfg.q, cfg.p_u);
pmf_v  = divisible_quant_error_pmf(cfg.q, cfg.p_v);

pmf_Lpk = linear_combination_pmf(pmf_pk, coeffs);
pmf_Ls  = linear_combination_pmf(pmf_s,  coeffs);
pmf_Lv  = linear_combination_pmf(pmf_v,  coeffs);

pmf_row_pk = product_pmf(pmf_Lpk, pmf_r);
pmf_row_u  = product_pmf(pmf_Ls,  pmf_u);

components = struct([]);
components(1).count = cfg.m;
components(1).supp = pmf_row_pk.supp;
components(1).prob = pmf_row_pk.prob;
components(2).count = cfg.n;
components(2).supp = -pmf_row_u.supp;
components(2).prob = pmf_row_u.prob;
components(3).count = 1;
components(3).supp = pmf_Lv.supp;
components(3).prob = pmf_Lv.prob;

stats = make_cgf_stats(components);
end

function stats = make_projection_stats_rowwise(cfg, coeffs)
% Row-wise projection <E(i,:), coeffs>.
% Used by compact-ciphertext profiles with ell_s = 8.
% For E = Epk^T R - S^T Eu + Ev, this gives
%   sum_a Epk(a,i) <R(a,:),coeffs>
% - sum_b S(b,i) <Eu(b,:),coeffs>
% + <Ev(i,:),coeffs>.
pmf_s  = centered_binomial_pmf(cfg.eta_s);
pmf_r  = centered_binomial_pmf(cfg.eta_r);
pmf_pk = divisible_quant_error_pmf(cfg.q, cfg.p_pk);
pmf_u  = divisible_quant_error_pmf(cfg.q, cfg.p_u);
pmf_v  = divisible_quant_error_pmf(cfg.q, cfg.p_v);

pmf_Lr = linear_combination_pmf(pmf_r, coeffs);
pmf_Lu = linear_combination_pmf(pmf_u, coeffs);
pmf_Lv = linear_combination_pmf(pmf_v, coeffs);

pmf_row_pk = product_pmf(pmf_pk, pmf_Lr);
pmf_row_u  = product_pmf(pmf_s,  pmf_Lu);

components = struct([]);
components(1).count = cfg.m;
components(1).supp = pmf_row_pk.supp;
components(1).prob = pmf_row_pk.prob;
components(2).count = cfg.n;
components(2).supp = -pmf_row_u.supp;
components(2).prob = pmf_row_u.prob;
components(3).count = 1;
components(3).supp = pmf_Lv.supp;
components(3).prob = pmf_Lv.prob;

stats = make_cgf_stats(components);
end

function stats = make_cgf_stats(components)
mu = 0;
min_supp = 0;
max_supp = 0;
for i = 1:numel(components)
    z = components(i).supp(:);
    p = components(i).prob(:);
    mu = mu + components(i).count * sum(z .* p);
    min_supp = min_supp + components(i).count * min(z);
    max_supp = max_supp + components(i).count * max(z);
end
stats = struct();
stats.components = components;
stats.mean = mu;
stats.min = min_supp;
stats.max = max_supp;
end

function out = cgf_eval(stats, theta)
% Returns [K(theta), K'(theta), K''(theta)].
K = 0;
K1 = 0;
K2 = 0;
for i = 1:numel(stats.components)
    comp = stats.components(i);
    z = comp.supp(:);
    p = comp.prob(:);
    a = theta .* z;
    amax = max(a);
    w = p .* exp(a - amax);
    Z = sum(w);
    mu = sum(w .* z) / Z;
    mu2 = sum(w .* z .* z) / Z;
    var = max(0, mu2 - mu * mu);
    K = K + comp.count * (amax + log(Z));
    K1 = K1 + comp.count * mu;
    K2 = K2 + comp.count * var;
end
out = [K, K1, K2];
end

function mu = cgf_mean(stats, theta)
s = cgf_eval(stats, theta);
mu = s(2);
end

% -------------------------------------------------------------------------
% Chernoff tails.
% -------------------------------------------------------------------------

function log2p = log2_chernoff_two_sided(stats, threshold)
log_right = chernoff_right_log(stats, threshold);
log_left = chernoff_left_log(stats, -threshold);
log2p = log2_sum([log_right, log_left] ./ log(2));
log2p = min(0, log2p);
end

function logp = chernoff_right_log(stats, x)
% Upper bound for P[X >= x].
if x > stats.max
    logp = -inf;
    return;
end
if x <= stats.mean
    logp = 0;
    return;
end

lo = 0;
hi = 1e-8;
while cgf_mean(stats, hi) < x
    hi = hi * 2;
    if hi > 100
        break;
    end
end

if hi > 100 && cgf_mean(stats, hi) < x
    objective = @(theta) cgf_eval(stats, theta) * [1;0;0] - theta * x;
    [theta, val] = fminbnd(objective, 0, 100);
    %#ok<NASGU>
    logp = min(0, val);
    return;
end

theta = fzero(@(u) cgf_mean(stats, u) - x, [lo, hi]);
s = cgf_eval(stats, theta);
logp = min(0, s(1) - theta * x);
end

function logp = chernoff_left_log(stats, x)
% Upper bound for P[X <= x].
if x < stats.min
    logp = -inf;
    return;
end
if x >= stats.mean
    logp = 0;
    return;
end

lo = -1e-8;
hi = 0;
while cgf_mean(stats, lo) > x
    lo = lo * 2;
    if lo < -100
        break;
    end
end

if lo < -100 && cgf_mean(stats, lo) > x
    objective = @(theta) cgf_eval(stats, theta) * [1;0;0] - theta * x;
    [theta, val] = fminbnd(objective, -100, 0);
    %#ok<NASGU>
    logp = min(0, val);
    return;
end

theta = fzero(@(u) cgf_mean(stats, u) - x, [lo, hi]);
s = cgf_eval(stats, theta);
logp = min(0, s(1) - theta * x);
end

% -------------------------------------------------------------------------
% PMF constructors and small-support algebra.
% -------------------------------------------------------------------------

function pmf = centered_binomial_pmf(eta)
supp = (-eta:eta).';
prob = zeros(size(supp));
den = 2^(2 * eta);
for a = 0:eta
    ca = nchoosek(eta, a);
    for b = 0:eta
        cb = nchoosek(eta, b);
        idx = (a - b) - supp(1) + 1;
        prob(idx) = prob(idx) + (ca * cb) / den;
    end
end
pmf = make_pmf(supp, prob);
end

function pmf = divisible_quant_error_pmf(q, p)
Delta = q / p;
if Delta ~= floor(Delta)
    error('Delta must be an integer.');
end
supp_map = containers.Map('KeyType', 'char', 'ValueType', 'double');
for u = 0:(Delta - 1)
    t = floor(u / Delta + 0.5) * Delta;
    e = t - u;
    key = sprintf('%d', e);
    if isKey(supp_map, key)
        supp_map(key) = supp_map(key) + 1 / Delta;
    else
        supp_map(key) = 1 / Delta;
    end
end
keys_cell = keys(supp_map);
supp = zeros(numel(keys_cell), 1);
prob = zeros(numel(keys_cell), 1);
for i = 1:numel(keys_cell)
    supp(i) = str2double(keys_cell{i});
    prob(i) = supp_map(keys_cell{i});
end
pmf = make_pmf(supp, prob);
end

function pmf = linear_combination_pmf(base_pmf, coeffs)
pmf = make_pmf(0, 1);
for i = 1:numel(coeffs)
    shifted = make_pmf(coeffs(i) * base_pmf.supp, base_pmf.prob);
    pmf = convolve_sparse_pmfs(pmf, shifted);
end
end

function pmf = product_pmf(pmf1, pmf2)
vals = [];
probs = [];
for i = 1:numel(pmf1.supp)
    for j = 1:numel(pmf2.supp)
        vals(end+1,1) = pmf1.supp(i) * pmf2.supp(j); %#ok<AGROW>
        probs(end+1,1) = pmf1.prob(i) * pmf2.prob(j); %#ok<AGROW>
    end
end
pmf = make_pmf(vals, probs);
end

function pmf = convolve_sparse_pmfs(pmf1, pmf2)
vals = [];
probs = [];
for i = 1:numel(pmf1.supp)
    for j = 1:numel(pmf2.supp)
        vals(end+1,1) = pmf1.supp(i) + pmf2.supp(j); %#ok<AGROW>
        probs(end+1,1) = pmf1.prob(i) * pmf2.prob(j); %#ok<AGROW>
    end
end
pmf = make_pmf(vals, probs);
end

function pmf = make_pmf(supp, prob)
supp = supp(:);
prob = prob(:);
[su, ~, ic] = unique(supp);
pu = zeros(size(su));
for i = 1:numel(ic)
    pu(ic(i)) = pu(ic(i)) + prob(i);
end
total = sum(pu);
if total <= 0
    error('PMF total mass must be positive.');
end
pu = pu / total;
keep = pu > 0;
pmf = struct('supp', su(keep), 'prob', pu(keep));
end

function y = log2_sum(log2_values)
finite = log2_values(isfinite(log2_values));
if isempty(finite)
    y = -inf;
    return;
end
m = max(finite);
y = m + log2(sum(2 .^ (finite - m)));
end
