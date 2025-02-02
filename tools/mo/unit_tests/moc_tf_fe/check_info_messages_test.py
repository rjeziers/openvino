# Copyright (C) 2018-2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import argparse
import io
import os
import unittest
from contextlib import redirect_stdout
from unittest.mock import patch

from openvino.tools.mo.main import main
from openvino.tools.ovc.get_ov_update_message import get_tf_fe_message


def arg_parse_helper(input_model,
                     use_legacy_frontend,
                     use_new_frontend,
                     input_model_is_text,
                     framework,
                     compress_to_fp16=False,
                     freeze_placeholder_with_value=None):
    path = os.path.dirname(__file__)
    input_model = os.path.join(path, "test_models", input_model)

    return argparse.Namespace(
        input_model=input_model,
        use_legacy_frontend=use_legacy_frontend,
        use_new_frontend=use_new_frontend,
        framework=framework,
        input_model_is_text=input_model_is_text,
        log_level='INFO',
        silent=True,
        model_name=None,
        transform=[],
        scale=None,
        output=None,
        input=None,
        input_shape=None,
        batch=None,
        input_checkpoint=None,
        saved_model_dir=None,
        input_meta_graph=None,
        saved_model_tags=None,
        output_dir='.',
        mean_values=(),
        scale_values=(),
        layout={},
        source_layout={},
        target_layout={},
        freeze_placeholder_with_value=freeze_placeholder_with_value,
        data_type=None,
        tensorflow_custom_operations_config_update=None,
        compress_to_fp16=compress_to_fp16,
        extensions=None
    )


class TestInfoMessagesTFFEWithFallback(unittest.TestCase):
    @patch('argparse.ArgumentParser.parse_args',
           return_value=arg_parse_helper(input_model="model_switch_merge.pbtxt",
                                         use_legacy_frontend=False, use_new_frontend=False,
                                         framework=None, input_model_is_text=True,
                                         freeze_placeholder_with_value="is_training->False"))
    def test_tf_fe_message_fallback(self, mock_argparse):
        f = io.StringIO()
        with redirect_stdout(f):
            main(argparse.ArgumentParser())
            std_out = f.getvalue()
        tf_fe_message_found = get_tf_fe_message() in std_out
        assert not tf_fe_message_found, 'TF FE Info message is found for the fallback case'

