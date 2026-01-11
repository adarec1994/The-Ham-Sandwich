var shaderView = {
    show: function (fileName) {
        var code = API.filesystem.loadShader(fileName);
        var permutations = JSON.parse(code);
        if (permutations.length == 0) {
            return;
        }

        $('#codeGlView_content').text(permutations[0]);
        $('#codeGlView_content').removeClass();
        $('#codeGlView_content').addClass('hlsl');
        $('#codeGlView_content').each(function (i, e) { hljs.highlightBlock(e); });

        $('#codeGlView').css({ 'display': 'block' });
    },

    hide: function () {
        $('#codeGlView').css({ 'display': 'none' });
    }
}

$(document).ready(function () {
    hljs.LANGUAGES.hlsl = {
        k: 'loop dcl_constantbuffer dcl_sampler dcl_resource_texture2d dcl_input_ps dcl_output dcl_temps sample mul mov ige breakc_nz mad iadd endloop ret ' +
           'and break breakc call callc case continue continuec cut dcl_globalFlags dcl_immediateConstantBuffer dcl_indexableTemp dcl_indexRange dcl_input_sv ' +
           'dcl_maxOutputVertexCount dcl_output_oDepth dcl_output_sgv dcl_output_siv decl_outputTopology dcl_temps default deriv_rtx deriv_rty discard div dp2 ' +
           'dp3 dp4 else emit emitThenCut endif endswitch eq exp frc ftoi ftou ge ibfe ieq if ilt imad imin imul ine ineg ishl ishr itof label ld log lt max min ' +
           'movc ne nop not or resinfo retc round_ne round_ni round_pi round_z rsq sample_b sample_c sample_c_lz sample_d sample_l sincos sqrt switch udiv uge ult ' +
           'umad umax umin umul ushr utof xor dcl_input mad_sat add dcl_input_ps_siv mul_sat dcl_input if_nz',
        c: [{
            cN: 'comment',
            b: '//',
            e: '\n'
        }, {
            cN: 'number',
            b: '\\b(\\d+(\\.\\d+)?)|(0x\d{1,8})'
        }, {
            cN: 'function',
            b: '\\b(r|o|t|s|cb|v)\\d{1,2}'
        }, {
            cN: 'built_in',
            b: '\\b(float|immediateIndexed|mode_default|linear|ps_4_0|vs_4_0|noperspective)'
        }]
    };
});

viewController.extensionMap.sho = shaderView;