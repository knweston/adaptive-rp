import os
import sys
import itertools
import argparse

def create_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

class SlurmScript:

    def __init__(self, cluster, job_name, job_dir, out_dir, cmd, time="72:00:00", mem="16G", num_threads="6"):
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
        elif self.cluster == "grace":
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



def generate_jobs(apps, output, cluster, batch_no, executable):
    # parse requested apps
    requested_apps = apps.split("_")

    if apps == "all":
        requested_apps = ["602", "605", "607", "619", "620", "621", \
                          "627", "628", "649", "410", "429", "450", \
                          "462", "470", "471", "482"]

    # get user's home directory
    home_dir   = os.path.dirname(os.getcwd())

    # parse code/trace/output/jobfile/config directories
    code_dir   = os.path.join(os.getcwd(), 'lite-sim')
    trace_dir  = os.path.join(home_dir, 'lite-traces')  # llc-traces directory
    job_dir    = os.path.join(home_dir, 'data/slurm-jobs')
    output_dir = os.path.join(os.path.join(home_dir, 'data/result'), output+'_b'+batch_no)
    config_dir = os.path.join(code_dir, 'zpl-module/config')

    # create jobfile/output directories if not exist
    create_dir(job_dir)
    create_dir(output_dir)

    job_list = []
    # construct commands
    for idx, app in enumerate(requested_apps):
        config_file = config_dir+'/'+app+'/'+batch_no+'.ini'
        cmd = f"python3 {os.path.join(code_dir, 'zpl-module/main.py')} {app} none {config_file} {32} {32} > python-{app}-b{batch_no}.txt & \n"
        cmd += f"sleep 250\n"
        cmd += f"for i in {'{1..2}'}\n"
        cmd += f"do\n"
        cmd += f"    {code_dir+'/'}{executable} ZPL NORMAL {trace_dir+'/'+app+'.LLC-TRACE.txt'} 0 10000 {config_file}\n"
        cmd += f"done\n"

        new_script = SlurmScript(cluster, app, job_dir, output_dir, cmd)
        new_script.create()
        job_list.append(new_script)
    
    return job_list
    


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--applications", required=True, type=str)
    parser.add_argument("-o", "--output_folder", required=True, type=str)
    parser.add_argument("-c", "--cluster_name", required=True, type=str)
    parser.add_argument("-b", "--batch_no", required=True, type=str)
    parser.add_argument("-e", "--exec_file", required=True, type=str)
    args = parser.parse_args()
    job_list = generate_jobs(args.applications, args.output_folder, args.cluster_name, args.batch_no, args.exec_file)
    for job in job_list:
        job.submit()


if __name__ == '__main__':
    main()