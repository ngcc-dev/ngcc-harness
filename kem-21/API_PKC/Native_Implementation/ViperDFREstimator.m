% viper_power2_dfr_e8_all_profiles_z_eq_k.m
% DFR worksheet for power-of-two MAMBA-Viper profiles.
%
% This version supports E8-coded message layers for all five profiles.
%
% Length convention in this version:
%   FO message bits = shared secret bits = target security bits.
%   z bits          = shared secret bits = target security bits.
%   The fallback secret z is therefore not fixed to 32 bytes.
%
% Scalar decryption-error model:
%
%   E = e_pk^T r - s^T e_u + e_v.
%
% Quantization-error distribution:
%
%   chi_{q,p} = Uniform{-Delta/2+1, ..., Delta/2},
%   Delta = q/p.
%
% The distribution is not centered.
%
% E8 mode:
%   For rate R bits/dimension, the fine lattice is alpha * E8 with
%
%      alpha = q / 2^R.
%
%   Each E8 block carries 8R bits.
%   The script computes exact projected-facet tail probabilities by
%   discrete convolution and then applies a union bound over E8 Voronoi facets.
%
% This is not a Gaussian or Chernoff approximation.
% The only relaxation is the E8 Voronoi-facet union bound.

clear;
clc;
format long g;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% User options
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RUN_SELECTED_PRESETS = true;
RUN_CANDIDATE_GRID   = false;
WRITE_CSV            = true;
CSV_FILE             = 'viper_power2_dfr_e8_all_profiles_results.csv';

% To run all selected presets, use 1:5.
SELECTED_PRESET_INDICES = 1:5;

ENABLE_E8_CODEC = true;
COMPARE_SCALAR_FOR_E8 = true;

INCLUDE_POSITIVE_BOUNDARY = true;
INCLUDE_NEGATIVE_BOUNDARY = true;

GAUSS_SCREEN_LOG2_WHOLE_MAX = -120;

n = 256;

SEED_BYTES = 32;
HPK_BYTES  = 32;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Selected presets
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Columns:
%   name, target, q, ss_bits, k, eta_s, eta_r, t_pk, t_u, t_v,
%   scalar_b_msg, v_count, use_e8, e8_rate, e8_message_bits, status
%
% In this script, the effective FO message length is P.ss_bits.
% P.e8_message_bits is retained as a codec-layout field and checked against
% P.ss_bits for selected E8 presets.
%
% E8 convention:
%   e8_rate = 1:
%      alpha = q/2, 8 bits per E8 block.
%      Used below for 128/192/256.
%
%   e8_rate = 2:
%      alpha = q/4, 16 bits per E8 block.
%      Used below for 384/512.

params = empty_param_array();

params = append_param(params, 'Viper-128', 128, 4096, 128, ...
    2, 2, 2, 9, 9, 4, ...
    1, 1, true, 1, 128, 'frozen');

params = append_param(params, 'Viper-192', 192, 4096, 192, ...
    3, 3, 3, 10, 9, 6, ...
    1, 1, true, 1, 192, 'frozen');

params = append_param(params, 'Viper-256', 256, 4096, 256, ...
    4, 3, 3, 10, 10, 5, ...
    1, 1, true, 1, 256, 'frozen');

params = append_param(params, 'Viper-384', 384, 8192, 384, ...
    7, 1, 1, 11, 11, 5, ...
    2, 1, true, 2, 384, 'frozen');

params = append_param(params, 'Viper-512', 512, 8192, 512, ...
    9, 1, 1, 11, 11, 8, ...
    2, 1, true, 2, 512, 'frozen');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Optional candidate grid
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if RUN_CANDIDATE_GRID
    grid_params = empty_param_array();

    targets = [128, 192, 256, 384, 512];

    for tt = 1:length(targets)
        target = targets(tt);

        if target == 128
            q_grid = 4096;
            ks = [2, 3];
            ss_bits = 128;
            eta_vals = [2, 3];
            t_pk_vals = [9, 10, 11];
            t_u_vals  = [9, 10, 11];
            t_v_vals  = [5, 6, 7, 8];
            e8_rate = 1;
            e8_message_bits = ss_bits;
        elseif target == 192
            q_grid = 4096;
            ks = [3, 4];
            ss_bits = 192;
            eta_vals = [2, 3];
            t_pk_vals = [9, 10, 11];
            t_u_vals  = [9, 10, 11];
            t_v_vals  = [5, 6, 7, 8];
            e8_rate = 1;
            e8_message_bits = ss_bits;
        elseif target == 256
            q_grid = 4096;
            ks = [4, 5];
            ss_bits = 256;
            eta_vals = [2, 3];
            t_pk_vals = [9, 10, 11];
            t_u_vals  = [9, 10, 11];
            t_v_vals  = [5, 6, 7, 8];
            e8_rate = 1;
            e8_message_bits = ss_bits;
        elseif target == 384
            q_grid = 8192;
            ks = [6, 7, 8];
            ss_bits = 384;
            eta_vals = [1, 2, 3];
            t_pk_vals = [11, 12];
            t_u_vals  = [11, 12];
            t_v_vals  = [8, 9, 10, 11];
            e8_rate = 2;
            e8_message_bits = ss_bits;
        else
            q_grid = 8192;
            ks = [8, 9, 10];
            ss_bits = 512;
            eta_vals = [1, 2, 3];
            t_pk_vals = [11, 12];
            t_u_vals  = [11, 12];
            t_v_vals  = [8, 9, 10, 11];
            e8_rate = 2;
            e8_message_bits = ss_bits;
        end

        v_counts = [1];

        for k_idx = 1:length(ks)
            k = ks(k_idx);

            for eta_s = eta_vals
                for eta_r = eta_vals
                    for t_pk = t_pk_vals
                        for t_u = t_u_vals
                            for t_v = t_v_vals
                                for v_count = v_counts
                                    name = sprintf('Grid-%d-q%d-k%d-es%d-er%d-t%d%d%d-E8r%d', ...
                                        target, q_grid, k, eta_s, eta_r, t_pk, t_u, t_v, e8_rate);

                                    grid_params = append_param(grid_params, name, target, q_grid, ss_bits, ...
                                        k, eta_s, eta_r, t_pk, t_u, t_v, ...
                                        e8_rate, v_count, true, e8_rate, e8_message_bits, 'grid'); %#ok<SAGROW>
                                end
                            end
                        end
                    end
                end
            end
        end
    end

    params = [params, grid_params];
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Main evaluation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

fprintf('Viper DFR worksheet with E8 support for all profiles\n');
fprintf('n = %d\n', n);
fprintf('DFR model: E = e_pk^T r - s^T e_u + e_v\n');
fprintf('Length convention: FO message bits = shared secret bits, z bits = shared secret bits\n');
fprintf('E8 mode: exact scalar convolution + exact projected E8 facet convolutions + union bound\n');
fprintf('Boundary convention: positive inclusive = %d, negative inclusive = %d\n', ...
    INCLUDE_POSITIVE_BOUNDARY, INCLUDE_NEGATIVE_BOUNDARY);
fprintf('ENABLE_E8_CODEC = %d\n\n', ENABLE_E8_CODEC);

rows = {};

for idx = 1:length(params)
    P = params(idx);

    if ~RUN_SELECTED_PRESETS && ~strcmp(P.status, 'grid')
        continue;
    end

    if ~strcmp(P.status, 'grid')
        if ~ismember(idx, SELECTED_PRESET_INDICES)
            continue;
        end
    end

    q = P.q;
    q_bits = round(log2(q));
    active_e8 = ENABLE_E8_CODEC && P.use_e8;

    fo_bits = P.ss_bits;
    fo_bytes = fo_bits / 8;
    ss_bytes = P.ss_bits / 8;
    z_bytes = ss_bytes;

    if active_e8
        if mod(n, 8) ~= 0
            fprintf('Skipping %s because E8 mode requires n divisible by 8.\n', P.name);
            continue;
        end

        if P.e8_rate <= 0
            fprintf('Skipping %s because E8 mode requires positive e8_rate.\n', P.name);
            continue;
        end

        bits_per_e8_block = 8 * P.e8_rate;

        if mod(fo_bits, bits_per_e8_block) ~= 0
            fprintf('Skipping %s because FO message bits is not a multiple of 8*e8_rate.\n', P.name);
            continue;
        end

        if P.e8_message_bits ~= P.ss_bits
            fprintf('Skipping %s because e8_message_bits must equal ss_bits in this length convention.\n', P.name);
            continue;
        end

        max_e8_blocks = (n / 8) * P.v_count;
        required_e8_blocks = fo_bits / bits_per_e8_block;

        if required_e8_blocks > max_e8_blocks
            fprintf('Skipping %s because required E8 blocks exceed available blocks.\n', P.name);
            continue;
        end
    end

    p_pk = 2^P.t_pk;
    p_u  = 2^P.t_u;
    p_v  = 2^P.t_v;

    if p_pk > q || p_u > q || p_v > q
        fprintf('Skipping %s because one compression modulus exceeds q.\n', P.name);
        continue;
    end

    if mod(q, p_pk) ~= 0 || mod(q, p_u) ~= 0 || mod(q, p_v) ~= 0
        fprintf('Skipping %s because this script requires p_x | q.\n', P.name);
        continue;
    end

    [vals_epk, prob_epk] = chi_power2_dist(q, p_pk);
    [vals_eu,  prob_eu]  = chi_power2_dist(q, p_u);
    [vals_ev,  prob_ev]  = chi_power2_dist(q, p_v);

    [vals_s, prob_s] = cbd_dist(P.eta_s);
    [vals_r, prob_r] = cbd_dist(P.eta_r);

    stat_epk = dist_stats(vals_epk, prob_epk);
    stat_eu  = dist_stats(vals_eu,  prob_eu);
    stat_ev  = dist_stats(vals_ev,  prob_ev);

    Nprod = P.k * n;

    var_s = P.eta_s / 2;
    var_r = P.eta_r / 2;

    % Correct diagnostic variance for non-centered quantization error.
    % Since r and s are centered, Var(e*r) = E[e^2] Var(r).
    m2_epk = stat_epk.var + stat_epk.mean^2;
    m2_eu  = stat_eu.var  + stat_eu.mean^2;

    approx_var = Nprod * m2_epk * var_r ...
               + Nprod * var_s * m2_eu ...
               + stat_ev.var;

    approx_sigma = sqrt(approx_var);

    scalar_threshold = q / 2^(P.scalar_b_msg + 1);

    approx_single = gaussian_two_sided_tail(scalar_threshold, approx_sigma);

    if active_e8
        scalar_decoded_coeffs = ceil(fo_bits / P.scalar_b_msg);
    else
        scalar_decoded_coeffs = n * P.v_count;
    end

    scalar_decoded_coeffs = min(scalar_decoded_coeffs, n * P.v_count);

    approx_whole = min(1, scalar_decoded_coeffs * approx_single);
    approx_log2_whole = log2_safe(approx_whole);

    if strcmp(P.status, 'grid') && approx_log2_whole > GAUSS_SCREEN_LOG2_WHOLE_MAX
        fprintf('Gaussian-screen warning %-60s approx log2 scalar DFR = %.4f\n', ...
            P.name, approx_log2_whole);
        fprintf('  Continuing exact convolution because Gaussian screen is diagnostic only.\n');
    end

    % Exact scalar product distributions.
    [vals_pk_prod, prob_pk_prod] = product_dist(vals_epk, prob_epk, vals_r, prob_r);

    [vals_u_prod, prob_u_prod] = product_dist(vals_s, prob_s, vals_eu, prob_eu);
    vals_u_prod = -vals_u_prod;
    [vals_u_prod, prob_u_prod] = normalize_dense(vals_u_prod, prob_u_prod);

    % Worst-case scalar support bound.
    [one_pk_min, one_pk_max] = product_support_bound(vals_epk, vals_r);
    [one_u_raw_min, one_u_raw_max] = product_support_bound(vals_s, vals_eu);

    one_u_min = -one_u_raw_max;
    one_u_max = -one_u_raw_min;

    worst_E_min = Nprod * one_pk_min + Nprod * one_u_min + min(vals_ev);
    worst_E_max = Nprod * one_pk_max + Nprod * one_u_max + max(vals_ev);

    % Exact scalar E distribution.
    [vals_sum_pk, prob_sum_pk] = dist_power(vals_pk_prod, prob_pk_prod, Nprod);
    [vals_sum_u,  prob_sum_u]  = dist_power(vals_u_prod,  prob_u_prod,  Nprod);

    [vals_total, prob_total] = conv_dist(vals_sum_pk, prob_sum_pk, vals_sum_u, prob_sum_u);
    [vals_total, prob_total] = conv_dist(vals_total, prob_total, vals_ev, prob_ev);

    stat_total = dist_stats(vals_total, prob_total);

    % Scalar diagnostic for the same scalar_b_msg.
    if INCLUDE_POSITIVE_BOUNDARY
        scalar_pos_tail = sum(prob_total(vals_total >= scalar_threshold));
    else
        scalar_pos_tail = sum(prob_total(vals_total > scalar_threshold));
    end

    if INCLUDE_NEGATIVE_BOUNDARY
        scalar_neg_tail = sum(prob_total(vals_total <= -scalar_threshold));
    else
        scalar_neg_tail = sum(prob_total(vals_total < -scalar_threshold));
    end

    scalar_single_dfr = scalar_pos_tail + scalar_neg_tail;
    scalar_whole_ub = min(1, scalar_decoded_coeffs * scalar_single_dfr);

    % E8 projected-facet DFR.
    e8_alpha = NaN;
    e8_rate = NaN;
    e8_bits_per_block = NaN;
    e8_blocks = 0;
    e8_pair_ub = NaN;
    e8_half_ub = NaN;
    e8_block_ub = NaN;
    e8_whole_ub = NaN;

    if active_e8
        e8_rate = P.e8_rate;
        e8_bits_per_block = 8 * e8_rate;
        e8_blocks = fo_bits / e8_bits_per_block;

        [e8_block_ub, e8_pair_ub, e8_half_ub, e8_alpha] = ...
            e8_facet_union_bound(vals_total, prob_total, q, e8_rate, INCLUDE_POSITIVE_BOUNDARY);

        e8_whole_ub = min(1, e8_blocks * e8_block_ub);
        whole_ub = e8_whole_ub;

        if e8_block_ub <= 0
            iid_heur = 0;
        elseif e8_block_ub >= 1
            iid_heur = 1;
        else
            iid_heur = -expm1(e8_blocks * log1p(-e8_block_ub));
        end

        codec_name = sprintf('E8-rate-%d', e8_rate);
    else
        whole_ub = scalar_whole_ub;

        if scalar_single_dfr <= 0
            iid_heur = 0;
        elseif scalar_single_dfr >= 1
            iid_heur = 1;
        else
            iid_heur = -expm1(scalar_decoded_coeffs * log1p(-scalar_single_dfr));
        end

        codec_name = 'scalar';
    end

    pk_bytes = SEED_BYTES + P.k * n * P.t_pk / 8;
    ct_bytes = SEED_BYTES + P.k * n * P.t_u / 8 + P.v_count * n * P.t_v / 8;
    sk_bytes = P.k * n * q_bits / 8 + pk_bytes + HPK_BYTES + z_bytes;

    fprintf('============================================================\n');
    fprintf('%s [%s]\n', P.name, P.status);
    fprintf('codec                  = %s\n', codec_name);
    fprintf('q                      = %d\n', q);
    fprintf('q bits                 = %d\n', q_bits);
    fprintf('target strength        = %d\n', P.target);
    fprintf('shared secret bits     = %d\n', P.ss_bits);
    fprintf('FO message bits        = %d\n', fo_bits);
    fprintf('FO message bytes       = %.0f\n', fo_bytes);
    fprintf('z bytes                = %.0f\n', z_bytes);
    fprintf('k                      = %d\n', P.k);
    fprintf('eta_s, eta_r           = (%d,%d)\n', P.eta_s, P.eta_r);
    fprintf('t_pk, t_u, t_v         = (%d,%d,%d)\n', P.t_pk, P.t_u, P.t_v);
    fprintf('p_pk, p_u, p_v         = (%d,%d,%d)\n', p_pk, p_u, p_v);
    fprintf('Delta_pk,u,v           = (%d,%d,%d)\n', q / p_pk, q / p_u, q / p_v);
    fprintf('product terms          = k*n = %d\n', Nprod);

    if active_e8
        fprintf('E8 rate bits/dim       = %d\n', e8_rate);
        fprintf('E8 bits/block          = %d\n', e8_bits_per_block);
        fprintf('E8 active blocks       = %d\n', e8_blocks);
        fprintf('E8 alpha               = %.12g\n', e8_alpha);
    else
        fprintf('scalar b_msg           = %d\n', P.scalar_b_msg);
        fprintf('scalar decoded coeffs  = %d\n', scalar_decoded_coeffs);
    end

    fprintf('support e_pk           = [%d,%d]\n', stat_epk.min, stat_epk.max);
    fprintf('support e_u            = [%d,%d]\n', stat_eu.min, stat_eu.max);
    fprintf('support e_v            = [%d,%d]\n', stat_ev.min, stat_ev.max);

    fprintf('mean e_pk,e_u,e_v      = (%.12g, %.12g, %.12g)\n', ...
        stat_epk.mean, stat_eu.mean, stat_ev.mean);

    fprintf('var e_pk,e_u,e_v       = (%.12g, %.12g, %.12g)\n', ...
        stat_epk.var, stat_eu.var, stat_ev.var);

    fprintf('mean E                 = %.12g\n', stat_total.mean);
    fprintf('var E                  = %.12g\n', stat_total.var);
    fprintf('effective support E    = [%d,%d]\n', stat_total.min, stat_total.max);
    fprintf('worst-case support E   = [%d,%d]\n', worst_E_min, worst_E_max);

    fprintf('approx sigma(E)        = %.12g\n', approx_sigma);
    fprintf('approx log2 scalar DFR = %.8f\n', approx_log2_whole);

    if active_e8
        fprintf('E8 pair-facet UB       = %.16e\n', e8_pair_ub);
        fprintf('log2 E8 pair UB        = %.8f\n', log2_safe(e8_pair_ub));
        fprintf('E8 half-facet UB       = %.16e\n', e8_half_ub);
        fprintf('log2 E8 half UB        = %.8f\n', log2_safe(e8_half_ub));
        fprintf('E8 block UB            = %.16e\n', e8_block_ub);
        fprintf('log2 E8 block UB       = %.8f\n', log2_safe(e8_block_ub));
        fprintf('whole-ct E8 UB DFR     = %.16e\n', whole_ub);
        fprintf('log2 whole E8 UB DFR   = %.8f\n', log2_safe(whole_ub));

        if COMPARE_SCALAR_FOR_E8
            fprintf('--- scalar diagnostic for same message length ---\n');
            fprintf('scalar b_msg            = %d\n', P.scalar_b_msg);
            fprintf('scalar threshold        = %.12g\n', scalar_threshold);
            fprintf('scalar decoded coeffs   = %d\n', scalar_decoded_coeffs);
            fprintf('scalar positive tail    = %.16e\n', scalar_pos_tail);
            fprintf('scalar negative tail    = %.16e\n', scalar_neg_tail);
            fprintf('scalar single DFR       = %.16e\n', scalar_single_dfr);
            fprintf('log2 scalar single DFR  = %.8f\n', log2_safe(scalar_single_dfr));
            fprintf('scalar whole UB DFR     = %.16e\n', scalar_whole_ub);
            fprintf('log2 scalar whole UB    = %.8f\n', log2_safe(scalar_whole_ub));
        end
    else
        fprintf('scalar threshold        = %.12g\n', scalar_threshold);
        fprintf('positive tail           = %.16e\n', scalar_pos_tail);
        fprintf('log2 positive tail      = %.8f\n', log2_safe(scalar_pos_tail));
        fprintf('negative tail           = %.16e\n', scalar_neg_tail);
        fprintf('log2 negative tail      = %.8f\n', log2_safe(scalar_neg_tail));
        fprintf('single-coeff DFR        = %.16e\n', scalar_single_dfr);
        fprintf('log2 single DFR         = %.8f\n', log2_safe(scalar_single_dfr));
        fprintf('whole-ct UB DFR         = %.16e\n', whole_ub);
        fprintf('log2 whole-ct UB DFR    = %.8f\n', log2_safe(whole_ub));
    end

    fprintf('whole-ct iid heuristic  = %.16e\n', iid_heur);
    fprintf('log2 iid heuristic      = %.8f\n', log2_safe(iid_heur));

    fprintf('pk, ct, sk, ss bytes    = (%.0f, %.0f, %.0f, %.0f)\n', ...
        pk_bytes, ct_bytes, sk_bytes, ss_bytes);

    rows(end + 1, :) = {P.name, P.status, codec_name, q, q_bits, P.target, P.ss_bits, fo_bits, z_bytes, ...
        P.k, P.eta_s, P.eta_r, P.t_pk, P.t_u, P.t_v, P.scalar_b_msg, P.v_count, ...
        P.use_e8, active_e8, P.e8_rate, P.e8_message_bits, ...
        p_pk, p_u, p_v, q / p_pk, q / p_u, q / p_v, Nprod, ...
        stat_epk.mean, stat_eu.mean, stat_ev.mean, ...
        stat_epk.var, stat_eu.var, stat_ev.var, ...
        stat_total.mean, stat_total.var, stat_total.min, stat_total.max, ...
        worst_E_min, worst_E_max, approx_sigma, ...
        scalar_threshold, scalar_decoded_coeffs, ...
        scalar_single_dfr, log2_safe(scalar_single_dfr), ...
        scalar_whole_ub, log2_safe(scalar_whole_ub), ...
        e8_alpha, e8_blocks, e8_pair_ub, log2_safe(e8_pair_ub), ...
        e8_half_ub, log2_safe(e8_half_ub), ...
        e8_block_ub, log2_safe(e8_block_ub), ...
        whole_ub, log2_safe(whole_ub), iid_heur, log2_safe(iid_heur), ...
        pk_bytes, ct_bytes, sk_bytes, ss_bytes}; %#ok<SAGROW>
end

fprintf('============================================================\n');
fprintf('Done.\n');

if WRITE_CSV && ~isempty(rows)
    headers = {'name', 'status', 'codec', 'q', 'q_bits', 'target', 'ss_bits', 'fo_bits', 'z_bytes', ...
        'k', 'eta_s', 'eta_r', 't_pk', 't_u', 't_v', 'scalar_b_msg', 'v_count', ...
        'use_e8_flag', 'active_e8', 'e8_rate', 'e8_message_bits', ...
        'p_pk', 'p_u', 'p_v', 'Delta_pk', 'Delta_u', 'Delta_v', 'Nprod', ...
        'epk_mean', 'eu_mean', 'ev_mean', ...
        'epk_var', 'eu_var', 'ev_var', ...
        'E_mean', 'E_var', 'E_effective_min', 'E_effective_max', ...
        'E_worst_min', 'E_worst_max', 'approx_sigma', ...
        'scalar_threshold', 'scalar_decoded_coeffs', ...
        'scalar_single_dfr', 'log2_scalar_single_dfr', ...
        'scalar_whole_ub', 'log2_scalar_whole_ub', ...
        'e8_alpha', 'e8_blocks', 'e8_pair_ub', 'log2_e8_pair_ub', ...
        'e8_half_ub', 'log2_e8_half_ub', ...
        'e8_block_ub', 'log2_e8_block_ub', ...
        'whole_ub', 'log2_whole_ub', 'iid_heur', 'log2_iid_heur', ...
        'pk_bytes', 'ct_bytes', 'sk_bytes', 'ss_bytes'};

    T = cell2table(rows, 'VariableNames', headers);
    writetable(T, CSV_FILE);
    fprintf('CSV written to %s\n', CSV_FILE);
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Local functions
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function params = empty_param_array()
    prototype = struct( ...
        'name', '', ...
        'target', 0, ...
        'q', 0, ...
        'ss_bits', 0, ...
        'k', 0, ...
        'eta_s', 0, ...
        'eta_r', 0, ...
        't_pk', 0, ...
        't_u', 0, ...
        't_v', 0, ...
        'scalar_b_msg', 0, ...
        'v_count', 0, ...
        'use_e8', false, ...
        'e8_rate', 0, ...
        'e8_message_bits', 0, ...
        'status', '');

    params = repmat(prototype, 0, 1);
end

function params = append_param(params, name, target, q, ss_bits, k, eta_s, eta_r, ...
    t_pk, t_u, t_v, scalar_b_msg, v_count, use_e8, e8_rate, e8_message_bits, status)

    params(end + 1, 1) = make_param(name, target, q, ss_bits, k, eta_s, eta_r, ...
        t_pk, t_u, t_v, scalar_b_msg, v_count, use_e8, e8_rate, e8_message_bits, status);
end

function P = make_param(name, target, q, ss_bits, k, eta_s, eta_r, ...
    t_pk, t_u, t_v, scalar_b_msg, v_count, use_e8, e8_rate, e8_message_bits, status)

    P = struct( ...
        'name', name, ...
        'target', target, ...
        'q', q, ...
        'ss_bits', ss_bits, ...
        'k', k, ...
        'eta_s', eta_s, ...
        'eta_r', eta_r, ...
        't_pk', t_pk, ...
        't_u', t_u, ...
        't_v', t_v, ...
        'scalar_b_msg', scalar_b_msg, ...
        'v_count', v_count, ...
        'use_e8', use_e8, ...
        'e8_rate', e8_rate, ...
        'e8_message_bits', e8_message_bits, ...
        'status', status);
end

function [vals, probs] = chi_power2_dist(q, p)
    Delta = q / p;

    if abs(Delta - round(Delta)) > 0
        error('chi_power2_dist requires p | q.');
    end

    if Delta == 1
        vals = 0;
        probs = 1;
        return;
    end

    if mod(Delta, 2) ~= 0
        error('This power-of-two branch expects even Delta.');
    end

    lo = -Delta / 2 + 1;
    hi =  Delta / 2;

    vals = lo:hi;
    probs = ones(size(vals)) / Delta;
    probs = probs / sum(probs);
end

function [vals, probs] = cbd_dist(eta)
    vals = -eta:eta;
    probs = zeros(size(vals));
    denom = 2^(2 * eta);

    for i = 1:length(vals)
        x = vals(i);
        probs(i) = nchoosek(2 * eta, eta + x) / denom;
    end

    probs = probs / sum(probs);
end

function [vals, probs] = product_dist(vals_x, probs_x, vals_y, probs_y)
    [min_v, max_v] = product_support_bound(vals_x, vals_y);

    vals = min_v:max_v;
    probs = zeros(size(vals));

    for i = 1:length(vals_x)
        if probs_x(i) == 0
            continue;
        end

        for j = 1:length(vals_y)
            if probs_y(j) == 0
                continue;
            end

            v = vals_x(i) * vals_y(j);
            probs(v - min_v + 1) = probs(v - min_v + 1) + probs_x(i) * probs_y(j);
        end
    end

    probs = probs / sum(probs);
end

function [lo, hi] = product_support_bound(vals_x, vals_y)
    candidates = [
        vals_x(1)   * vals_y(1), ...
        vals_x(1)   * vals_y(end), ...
        vals_x(end) * vals_y(1), ...
        vals_x(end) * vals_y(end)
    ];

    lo = min(candidates);
    hi = max(candidates);
end

function [vals, probs] = normalize_dense(raw_vals, raw_probs)
    min_v = min(raw_vals);
    max_v = max(raw_vals);

    vals = min_v:max_v;
    probs = zeros(size(vals));

    for i = 1:length(raw_vals)
        pos = raw_vals(i) - min_v + 1;
        probs(pos) = probs(pos) + raw_probs(i);
    end

    probs(probs < 0) = 0;
    probs = probs / sum(probs);
end

function [vals_c, probs_c] = conv_dist(vals_a, probs_a, vals_b, probs_b)
    vals_c = (vals_a(1) + vals_b(1)):(vals_a(end) + vals_b(end));
    probs_c = conv(probs_a, probs_b);

    probs_c(probs_c < 0) = 0;

    s = sum(probs_c);
    if s > 0
        probs_c = probs_c / s;
    end
end

function [vals_out, probs_out] = dist_power(vals, probs, exponent)
    vals_out = 0;
    probs_out = 1;

    vals_base = vals;
    probs_base = probs;

    e = exponent;

    while e > 0
        if mod(e, 2) == 1
            [vals_out, probs_out] = conv_dist(vals_out, probs_out, vals_base, probs_base);
        end

        e = floor(e / 2);

        if e > 0
            [vals_base, probs_base] = conv_dist(vals_base, probs_base, vals_base, probs_base);
        end
    end
end

function st = dist_stats(vals, probs)
    mu = sum(vals .* probs);
    variance = sum(((vals - mu).^2) .* probs);

    st.mean = mu;
    st.var = variance;
    st.sigma = sqrt(variance);
    st.min = min(vals(probs > 0));
    st.max = max(vals(probs > 0));
end

function y = log2_safe(x)
    if isnan(x)
        y = NaN;
    elseif x <= 0
        y = -Inf;
    else
        y = log2(x);
    end
end

function p = gaussian_two_sided_tail(threshold, sigma)
    if sigma <= 0
        p = 0;
    else
        p = erfc(threshold / (sqrt(2) * sigma));
    end
end

function [block_ub, pair_ub, half_ub, alpha] = e8_facet_union_bound(vals_E, prob_E, q, e8_rate, include_boundary)
    % E8 rate-R setup:
    %   fine lattice   = alpha * E8
    %   coarse lattice = q Z^8
    %   code size per 8D block = (q/alpha)^8 = 2^(8R)
    % Hence alpha = q / 2^R.

    alpha = q / (2^e8_rate);

    if abs(alpha - round(alpha)) > 0
        error('E8 alpha must be integral for this discrete script.');
    end

    alpha = round(alpha);

    % Pair roots:
    %   r = (±1, ±1, 0, ..., 0)
    %   condition <E,r> >= alpha.
    [vals_sum2, prob_sum2] = conv_dist(vals_E, prob_E, vals_E, prob_E);

    vals_neg_E = -vals_E;
    [vals_neg_E, prob_neg_E] = normalize_dense(vals_neg_E, prob_E);

    [vals_diff2, prob_diff2] = conv_dist(vals_E, prob_E, vals_neg_E, prob_neg_E);

    p_pp = tail_ge(vals_sum2, prob_sum2, alpha, include_boundary);
    p_pm = tail_ge(vals_diff2, prob_diff2, alpha, include_boundary);
    p_mm = tail_le(vals_sum2, prob_sum2, -alpha, include_boundary);

    pair_ub = nchoosek(8, 2) * (p_pp + 2 * p_pm + p_mm);

    % Half roots:
    %   r = (±1/2, ..., ±1/2) with even number of minus signs.
    %   condition <E,r> >= alpha
    %   equivalently sum_i sign_i E_i >= 2 alpha.
    half_ub = 0;

    for m = [0, 2, 4, 6, 8]
        pos_count = 8 - m;
        neg_count = m;

        [vals_proj, prob_proj] = signed_sum_dist(vals_E, prob_E, pos_count, neg_count);
        p_m = tail_ge(vals_proj, prob_proj, 2 * alpha, include_boundary);

        half_ub = half_ub + nchoosek(8, m) * p_m;
    end

    block_ub = pair_ub + half_ub;
end

function [vals_out, probs_out] = signed_sum_dist(vals, probs, pos_count, neg_count)
    % Distribution of
    %   X_1 + ... + X_pos_count - Y_1 - ... - Y_neg_count
    % where all X_i,Y_i are iid with distribution (vals, probs).

    [vals_pos, probs_pos] = dist_power(vals, probs, pos_count);

    vals_neg = -vals;
    [vals_neg, probs_neg] = normalize_dense(vals_neg, probs);

    [vals_neg_sum, probs_neg_sum] = dist_power(vals_neg, probs_neg, neg_count);

    [vals_out, probs_out] = conv_dist(vals_pos, probs_pos, vals_neg_sum, probs_neg_sum);
end

function p = tail_ge(vals, probs, threshold, include_boundary)
    if include_boundary
        p = sum(probs(vals >= threshold));
    else
        p = sum(probs(vals > threshold));
    end
end

function p = tail_le(vals, probs, threshold, include_boundary)
    if include_boundary
        p = sum(probs(vals <= threshold));
    else
        p = sum(probs(vals < threshold));
    end
end
