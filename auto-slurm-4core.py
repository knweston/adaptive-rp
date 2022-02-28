import os
import sys
import argparse
import math

def create_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

class SlurmScript:
    def __init__(self, cluster, job_name, job_dir, out_dir, cmd, time="7-00:00:00", mem="8G", num_threads="4"):
        self.cluster  = cluster
        self.job_name = job_name
        self.job_dir  = job_dir
        self.out_dir  = out_dir
        self.cmd      = cmd
        self.time     = time
        self.mem      = mem
        self.ntasks   = num_threads

    def get_header(self):
        if self.cluster == "atlas":
            header = (
                f'#!/bin/bash\n'
                f'#SBATCH --job-name={self.job_name}\n'
                f'#SBATCH --output={self.out_dir}/{self.job_name}-%j.txt\n'
                f'#SBATCH --ntasks-per-node={self.ntasks}\n'
                f'#SBATCH --mem={self.mem}\n'
                f'\n'
            )
            return header
        elif self.cluster == "grace" or self.cluster == "terra":
            header = (
                f'#!/bin/bash\n'
                f'#SBATCH --job-name={self.job_name}\n'
                f'#SBATCH --output={self.out_dir}/{self.job_name}-%j.txt\n'
                f'#SBATCH --time={self.time}\n'
                f'#SBATCH --nodes=1\n'
                f'#SBATCH --ntasks-per-node={self.ntasks}\n'
                f'#SBATCH --mem={self.mem}\n'
                f'\n'
            )
            return header
    
    def create(self):
        script_path = self.job_dir+'/'+self.job_name+'.sh'
        with open(script_path, 'w') as f:
            f.write(self.get_header())
            f.write(self.cmd)
            print(f'Sbatch script is created: {script_path}', file=sys.stderr)

    def submit(self, prod=True):
        script_path = self.job_dir+'/'+self.job_name+'.sh'
        cmd = f'sbatch {script_path}'
        print(cmd)
        if prod:
            os.system(cmd)


def get_trace_path(input_name, trace_dir):
    post_fix = '.champsimtrace.xz'
    if input_name.find("mix") == -1:
        return input_name + post_fix
    else:
        trace_path = ""
        if input_name == "mix1":
            trace_path = trace_dir + "/602" + post_fix + " " + trace_dir + "/605" + post_fix + " " + trace_dir + "/607" + post_fix + " " + trace_dir + "/619" + post_fix
        elif input_name == "mix2":
            trace_path = trace_dir + "/620" + post_fix + " " + trace_dir + "/621" + post_fix + " " + trace_dir + "/623" + post_fix + " " + trace_dir + "/627" + post_fix
        elif input_name == "mix3":
            trace_path = trace_dir + "/628" + post_fix + " " + trace_dir + "/649" + post_fix + " " + trace_dir + "/638" + post_fix + " " + trace_dir + "/602" + post_fix
        elif input_name == "mix4":
            trace_path = trace_dir + "/605" + post_fix + " " + trace_dir + "/619" + post_fix + " " + trace_dir + "/621" + post_fix + " " + trace_dir + "/627" + post_fix
        elif input_name == "mix5":
            trace_path = trace_dir + "/605" + post_fix + " " + trace_dir + "/620" + post_fix + " " + trace_dir + "/628" + post_fix + " " + trace_dir + "/638" + post_fix
        elif input_name == "mix6":
            trace_path = trace_dir + "/607" + post_fix + " " + trace_dir + "/627" + post_fix + " " + trace_dir + "/628" + post_fix + " " + trace_dir + "/649" + post_fix
        elif input_name == "mix7":
            trace_path = trace_dir + "/607" + post_fix + " " + trace_dir + "/621" + post_fix + " " + trace_dir + "/627" + post_fix + " " + trace_dir + "/638" + post_fix
        elif input_name == "mix8":
            trace_path = trace_dir + "/605" + post_fix + " " + trace_dir + "/623" + post_fix + " " + trace_dir + "/628" + post_fix + " " + trace_dir + "/638" + post_fix
        elif input_name == "mix9":
            trace_path = trace_dir + "/607" + post_fix + " " + trace_dir + "/620" + post_fix + " " + trace_dir + "/623" + post_fix + " " + trace_dir + "/628" + post_fix
        elif input_name == "mix10":
            trace_path = trace_dir + "/602" + post_fix + " " + trace_dir + "/607" + post_fix + " " + trace_dir + "/621" + post_fix + " " + trace_dir + "/623" + post_fix
        elif input_name == "mix11":
            trace_path = trace_dir + "/605" + post_fix + " " + trace_dir + "/607" + post_fix + " " + trace_dir + "/627" + post_fix + " " + trace_dir + "/638" + post_fix
        elif input_name == "mix12":
            trace_path = trace_dir + "/602" + post_fix + " " + trace_dir + "/619" + post_fix + " " + trace_dir + "/623" + post_fix + " " + trace_dir + "/649" + post_fix
        else:
            exit(1)
        return trace_path 


def generate_jobs(apps, output, cluster, batch_no, executable, num_features, num_set, cache_levels):
    # parse requested apps
    requested_apps = apps.split("_")

    if apps == "all":
        requested_apps = ["mix1", "mix2", "mix3", "mix4", "mix5", "mix6", \
                          "mix7", "mix8", "mix9", "mix10", "mix11", "mix12"]

    # get user's home directory
    home_dir   = os.path.dirname(os.getcwd())

    # parse code/trace/output/jobfile/config directories
    code_dir   = os.path.join(os.getcwd(), 'final-sim')
    trace_dir  = os.path.join(home_dir, 'dpc3-traces')  # llc-traces directory
    job_dir    = os.path.join(home_dir, 'data/slurm-jobs')
    output_dir = os.path.join(os.path.join(home_dir, 'data/result'), output+'_b'+batch_no)
    config_dir = os.path.join(code_dir, 'dqn-module/config')

    # create jobfile/output directories if not exist
    create_dir(job_dir)
    create_dir(output_dir)

    job_list = []
    # construct commands
    for idx, app in enumerate(requested_apps):
        trace_path = get_trace_path(app, trace_dir)
        config_file = config_dir+'/'+app+'/'+batch_no+'.ini'

        set_list = [int(s) for s in num_set.split("_")]
        num_outputs = ""
        for i in range(len(set_list)):
            set_list[i] = int(32 - math.log2(set_list[i]) - 6 + 1)
            num_outputs += str(set_list[i]) + "_"
        num_outputs = num_outputs[:-1]

        cmd = f"python3 {os.path.join(code_dir, 'dqn-module/main.py')} -a {app} -w none -c {config_file} -f {num_features} -o {num_outputs} -l {cache_levels} {'&'}\n"
        cmd += f"sleep 250\n"
        cmd += f"for i in {'{1..190}'}\n"
        cmd += f"do\n"
        cmd += f"    {code_dir+'/run_4core.sh'} {code_dir+'/bin/'+executable} 50 250 {app} {config_file} {trace_path} \n"
        cmd += f"done\n"

        new_script = SlurmScript(cluster, app+'_b'+batch_no, job_dir, output_dir, cmd)
        new_script.create()
        job_list.append(new_script)
    
    return job_list
    

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--applications", required=True, type=str)
    parser.add_argument("-o", "--output_folder", required=True, type=str)
    parser.add_argument("-c", "--cluster_name", required=True, type=str)
    parser.add_argument("-b", "--batch_no", required=True, type=str)
    parser.add_argument("-f", "--num_features", required=True, type=str)
    parser.add_argument("-e", "--exec_file", required=True, type=str)
    parser.add_argument("-s", "--num_set", required=True, type=str)
    parser.add_argument("-l", "--cache_levels", required=True, type=str)
    args = parser.parse_args()

    job_list = generate_jobs(args.applications, args.output_folder, args.cluster_name, args.batch_no, args.exec_file, args.num_features, args.num_set, args.cache_levels)
    for job in job_list:
        job.submit()


if __name__ == '__main__':
    main()