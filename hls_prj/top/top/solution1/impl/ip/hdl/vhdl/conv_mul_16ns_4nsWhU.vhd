-- ==============================================================
-- Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC v2019.2 (64-bit)
-- Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
-- ==============================================================
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


entity conv_mul_16ns_4nsWhU_Mul_LUT_1 is
port (
    a: in std_logic_vector(16 - 1 downto 0);
    b: in std_logic_vector(4 - 1 downto 0);
    p: out std_logic_vector(20 - 1 downto 0));

    attribute use_dsp : string;
    attribute use_dsp of conv_mul_16ns_4nsWhU_Mul_LUT_1 : entity is "no";

end entity;

architecture behav of conv_mul_16ns_4nsWhU_Mul_LUT_1 is
    signal a_i : std_logic_vector(16 - 1 downto 0);
    signal b_i : std_logic_vector(4 - 1 downto 0);
begin
    a_i <= a;
    b_i <= b;
    p <= std_logic_vector(resize(unsigned(a_i) * unsigned(b_i), 20));
end architecture;
Library IEEE;
use IEEE.std_logic_1164.all;

entity conv_mul_16ns_4nsWhU is
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER);
    port (
        din0 : IN STD_LOGIC_VECTOR(din0_WIDTH - 1 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(din1_WIDTH - 1 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(dout_WIDTH - 1 DOWNTO 0));
end entity;

architecture arch of conv_mul_16ns_4nsWhU is
    component conv_mul_16ns_4nsWhU_Mul_LUT_1 is
        port (
            a : IN STD_LOGIC_VECTOR;
            b : IN STD_LOGIC_VECTOR;
            p : OUT STD_LOGIC_VECTOR);
    end component;



begin
    conv_mul_16ns_4nsWhU_Mul_LUT_1_U :  component conv_mul_16ns_4nsWhU_Mul_LUT_1
    port map (
        a => din0,
        b => din1,
        p => dout);

end architecture;


