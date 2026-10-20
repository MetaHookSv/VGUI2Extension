import importlib.util
from pathlib import Path
import unittest


spec = importlib.util.spec_from_file_location('career_validator', Path(__file__).parents[1] / 'validate-gamedata.py')
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)


class CareerStorageContractTests(unittest.TestCase):
    def fixture(self):
        records = []

        def add(name, kind, payload):
            records.append({'module': 'gameui', 'platform': 'windows',
                            'symbolName': name, 'kind': kind, 'payload': payload})

        for name, value in validator.CAREER_STORAGE_HL10210_SCALARS.items():
            add(name, 'scalar', {'scalar_name': name, 'scalar_value': value})
            if name.endswith('_count'):
                prefix = name.removesuffix('_count')
                for index in range(value):
                    add(f'{prefix}_{index}', 'patch', {'patch_rva': '0x2000'})
        for name, offset in validator.CAREER_STORAGE_HL10210_MEMBERS.items():
            struct, member = name.split('.')
            add(name, 'structMember', {'struct_name': struct, 'member_name': member, 'offset': hex(offset)})
        for name in ('CareerStorage_Definitions', 'CareerStorage_Game', 'CareerStorage_FileSystem'):
            add(name, 'global', {'gv_name': name, 'gv_rva': '0x1000', 'gv_va': '0x10001000',
                                 'gv_sig_va': '0x10002000', 'gv_sig': 'A1 ?? ?? ?? ??',
                                 'gv_inst_offset': '0x0', 'gv_inst_disp': '0x1', 'gv_inst_length': '0x5'})
        add('CCareerGame::CreateProfileBuffer(CCareerProfileData*, unsigned char*, int, bool)',
            'function', {'func_rva': '0x2000', 'func_size': '0x100'})
        add('vgui2::TextEntry::GetText(wchar_t*, int)', 'virtualFunction',
            {'vfunc_index': 136, 'vtable_name': 'vgui2::TextEntry', 'func_rva': '0x3000', 'func_size': '0x42'})
        snapshot = {'schemaVersion': 5,
                    'source': {'snapshotSchemaVersion': 8, 'analysisOutputContractVersion': 3},
                    'binaries': {'gameui': {'windows': {'crc64': '0000000000000001'}}},
                    'records': records}
        errors, _, result = validator.validate_snapshot(snapshot, 'hl-10210')
        self.assertEqual([], errors)
        return result

    def test_complete_contract_and_unaffected_non_career_identity(self):
        self.assertEqual([], validator.validate_career_storage(self.fixture(), 'hl-10210'))
        self.assertEqual([], validator.validate_career_storage({}, 'svencoop-10257'))

    def test_short_observation_cannot_be_advertised_as_branch_redirect(self):
        symbols = self.fixture()
        name = 'CCareerGame_BuildProfileList_ReadFile_callsite_0_hook_strategy'
        symbols['gameui', name]['value'] = 0
        self.assertTrue(validator.validate_career_storage(symbols, 'hl-10210'))

    def test_copy_gap_and_extra_site_fail_closed(self):
        prefix = 'CCareerProfileFrame_selected_name_copy_callsite_'
        for mutation in ('gap', 'extra'):
            with self.subTest(mutation=mutation):
                symbols = self.fixture()
                if mutation == 'gap':
                    del symbols['gameui', prefix+'1']
                else:
                    symbols['gameui', prefix+'2'] = {'kind': 'patch', 'payload': {}}
                self.assertTrue(validator.validate_career_storage(symbols, 'hl-10210'))

    def test_wrong_module_capacity_or_abi_is_rejected(self):
        name = 'CCareerGame_SaveProfile_profile_path_format_callsite_destination_capacity'
        for mutation in ('module', 'capacity', 'abi'):
            with self.subTest(mutation=mutation):
                symbols = self.fixture()
                if mutation == 'module':
                    symbols['client', name] = symbols.pop(('gameui', name))
                elif mutation == 'capacity':
                    symbols['gameui', name]['value'] = 32
                else:
                    symbols['gameui', 'CCareerProfileFrame_ResumeProfile_name_set_callsite_0_abi']['value'] = 1
                self.assertTrue(validator.validate_career_storage(symbols, 'hl-10210'))

    def test_unreviewed_identity_cannot_reuse_a_complete_contract(self):
        self.assertTrue(validator.validate_career_storage(self.fixture(), 'hl-8684'))

    def test_missing_preflight_and_changed_wide_getter_fail_closed(self):
        for name in ('CareerStorage_Definitions', 'CCareerDefinition.m_saveDir',
                     'BufferWriter.m_overflow', 'CareerStorage_runtime_contract_version'):
            with self.subTest(name=name):
                symbols = self.fixture()
                del symbols['gameui', name]
                self.assertTrue(validator.validate_career_storage(symbols, 'hl-10210'))
        symbols = self.fixture()
        symbols['gameui', 'vgui2::TextEntry::GetText(wchar_t*, int)']['vfuncIndex'] = 135
        self.assertTrue(validator.validate_career_storage(symbols, 'hl-10210'))


if __name__ == '__main__':
    unittest.main()
