# RootStream Infrastructure - AWS Terraform Configuration

terraform {
  required_version = ">= 1.0"
  
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.23"
    }
  }
  
  backend "s3" {
    bucket = "rootstream-terraform-state"
    key    = "infrastructure/terraform.tfstate"
    region = "us-east-1"
  }
}

provider "aws" {
  region = var.aws_region
}

# Data sources
data "aws_availability_zones" "available" {
  state = "available"
}

# VPC Configuration
resource "aws_vpc" "rootstream" {
  cidr_block           = "10.0.0.0/16"
  enable_dns_hostnames = true
  enable_dns_support   = true
  
  tags = {
    Name        = "rootstream-vpc"
    Environment = var.environment
    ManagedBy   = "Terraform"
  }
}

# Internet Gateway
resource "aws_internet_gateway" "rootstream" {
  vpc_id = aws_vpc.rootstream.id
  
  tags = {
    Name = "rootstream-igw"
  }
}

# Public Subnets
resource "aws_subnet" "public" {
  count             = 3
  vpc_id            = aws_vpc.rootstream.id
  cidr_block        = "10.0.${count.index + 1}.0/24"
  availability_zone = data.aws_availability_zones.available.names[count.index]
  
  map_public_ip_on_launch = true
  
  tags = {
    Name = "rootstream-public-subnet-${count.index + 1}"
    Type = "Public"
  }
}

# Private Subnets
resource "aws_subnet" "private" {
  count             = 3
  vpc_id            = aws_vpc.rootstream.id
  cidr_block        = "10.0.${count.index + 11}.0/24"
  availability_zone = data.aws_availability_zones.available.names[count.index]
  
  tags = {
    Name = "rootstream-private-subnet-${count.index + 1}"
    Type = "Private"
  }
}

# NAT Gateway
resource "aws_eip" "nat" {
  domain = "vpc"
  
  tags = {
    Name = "rootstream-nat-eip"
  }
}

resource "aws_nat_gateway" "rootstream" {
  allocation_id = aws_eip.nat.id
  subnet_id     = aws_subnet.public[0].id
  
  tags = {
    Name = "rootstream-nat-gw"
  }
  
  depends_on = [aws_internet_gateway.rootstream]
}

# Route Tables
resource "aws_route_table" "public" {
  vpc_id = aws_vpc.rootstream.id
  
  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = aws_internet_gateway.rootstream.id
  }
  
  tags = {
    Name = "rootstream-public-rt"
  }
}

resource "aws_route_table" "private" {
  vpc_id = aws_vpc.rootstream.id
  
  route {
    cidr_block     = "0.0.0.0/0"
    nat_gateway_id = aws_nat_gateway.rootstream.id
  }
  
  tags = {
    Name = "rootstream-private-rt"
  }
}

# Route Table Associations
resource "aws_route_table_association" "public" {
  count          = 3
  subnet_id      = aws_subnet.public[count.index].id
  route_table_id = aws_route_table.public.id
}

resource "aws_route_table_association" "private" {
  count          = 3
  subnet_id      = aws_subnet.private[count.index].id
  route_table_id = aws_route_table.private.id
}

# Security Groups
resource "aws_security_group" "eks_cluster" {
  name        = "rootstream-eks-cluster-sg"
  description = "Security group for EKS cluster"
  vpc_id      = aws_vpc.rootstream.id
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "rootstream-eks-cluster-sg"
  }
}

resource "aws_security_group" "rds" {
  name        = "rootstream-rds-sg"
  description = "Security group for RDS database"
  vpc_id      = aws_vpc.rootstream.id
  
  ingress {
    from_port       = 5432
    to_port         = 5432
    protocol        = "tcp"
    security_groups = [aws_security_group.eks_cluster.id]
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "rootstream-rds-sg"
  }
}

resource "aws_security_group" "redis" {
  name        = "rootstream-redis-sg"
  description = "Security group for Redis cache"
  vpc_id      = aws_vpc.rootstream.id
  
  ingress {
    from_port       = 6379
    to_port         = 6379
    protocol        = "tcp"
    security_groups = [aws_security_group.eks_cluster.id]
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "rootstream-redis-sg"
  }
}

resource "aws_security_group" "alb" {
  name        = "rootstream-alb-sg"
  description = "Security group for Application Load Balancer"
  vpc_id      = aws_vpc.rootstream.id
  
  ingress {
    from_port   = 80
    to_port     = 80
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  ingress {
    from_port   = 443
    to_port     = 443
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
  
  tags = {
    Name = "rootstream-alb-sg"
  }
}

# IAM Roles for EKS
resource "aws_iam_role" "eks_cluster" {
  name = "rootstream-eks-cluster-role"
  
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Principal = {
        Service = "eks.amazonaws.com"
      }
    }]
  })
}

resource "aws_iam_role_policy_attachment" "eks_cluster_policy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKSClusterPolicy"
  role       = aws_iam_role.eks_cluster.name
}

resource "aws_iam_role" "eks_node" {
  name = "rootstream-eks-node-role"
  
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Action = "sts:AssumeRole"
      Effect = "Allow"
      Principal = {
        Service = "ec2.amazonaws.com"
      }
    }]
  })
}

resource "aws_iam_role_policy_attachment" "eks_node_policy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKSWorkerNodePolicy"
  role       = aws_iam_role.eks_node.name
}

resource "aws_iam_role_policy_attachment" "eks_cni_policy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKS_CNI_Policy"
  role       = aws_iam_role.eks_node.name
}

resource "aws_iam_role_policy_attachment" "eks_container_registry_policy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEC2ContainerRegistryReadOnly"
  role       = aws_iam_role.eks_node.name
}

# EKS Cluster
resource "aws_eks_cluster" "rootstream" {
  name     = "rootstream-cluster"
  role_arn = aws_iam_role.eks_cluster.arn
  version  = var.kubernetes_version
  
  vpc_config {
    subnet_ids = concat(
      aws_subnet.public[*].id,
      aws_subnet.private[*].id
    )
    security_group_ids      = [aws_security_group.eks_cluster.id]
    endpoint_private_access = true
    endpoint_public_access  = true
  }
  
  depends_on = [
    aws_iam_role_policy_attachment.eks_cluster_policy
  ]
  
  tags = {
    Name = "rootstream-eks-cluster"
  }
}

# EKS Node Group
resource "aws_eks_node_group" "rootstream" {
  cluster_name    = aws_eks_cluster.rootstream.name
  node_group_name = "rootstream-nodes"
  node_role_arn   = aws_iam_role.eks_node.arn
  subnet_ids      = aws_subnet.private[*].id
  
  scaling_config {
    desired_size = var.node_desired_size
    max_size     = var.node_max_size
    min_size     = var.node_min_size
  }
  
  instance_types = [var.node_instance_type]
  
  update_config {
    max_unavailable = 1
  }
  
  depends_on = [
    aws_iam_role_policy_attachment.eks_node_policy,
    aws_iam_role_policy_attachment.eks_cni_policy,
    aws_iam_role_policy_attachment.eks_container_registry_policy,
  ]
  
  tags = {
    Name = "rootstream-eks-nodes"
  }
}

# RDS Subnet Group
resource "aws_db_subnet_group" "rootstream" {
  name       = "rootstream-db-subnet-group"
  subnet_ids = aws_subnet.private[*].id
  
  tags = {
    Name = "rootstream-db-subnet-group"
  }
}

# RDS PostgreSQL Instance
resource "aws_db_instance" "rootstream" {
  identifier           = "rootstream-db"
  engine              = "postgres"
  engine_version      = "15.3"
  instance_class      = var.db_instance_class
  allocated_storage   = var.db_allocated_storage
  storage_encrypted   = true
  
  db_name  = "rootstream"
  username = var.db_username
  password = var.db_password
  
  multi_az               = var.db_multi_az
  publicly_accessible    = false
  skip_final_snapshot    = false
  final_snapshot_identifier = "rootstream-final-snapshot-${formatdate("YYYY-MM-DD-hhmm", timestamp())}"
  
  vpc_security_group_ids = [aws_security_group.rds.id]
  db_subnet_group_name   = aws_db_subnet_group.rootstream.name
  
  backup_retention_period = 7
  backup_window          = "03:00-04:00"
  maintenance_window     = "mon:04:00-mon:05:00"
  
  tags = {
    Name = "rootstream-rds"
  }
}

# ElastiCache Subnet Group
resource "aws_elasticache_subnet_group" "rootstream" {
  name       = "rootstream-cache-subnet-group"
  subnet_ids = aws_subnet.private[*].id
}

# ElastiCache Redis Replication Group
resource "aws_elasticache_replication_group" "rootstream" {
  replication_group_id       = "rootstream-cache"
  replication_group_description = "RootStream Redis cache cluster"
  engine                     = "redis"
  engine_version            = "7.0"
  node_type                 = var.redis_node_type
  num_cache_clusters        = var.redis_num_nodes
  parameter_group_name      = "default.redis7"
  port                      = 6379
  
  subnet_group_name = aws_elasticache_subnet_group.rootstream.name
  security_group_ids = [aws_security_group.redis.id]
  
  at_rest_encryption_enabled = true
  transit_encryption_enabled = true
  
  automatic_failover_enabled = true
  
  tags = {
    Name = "rootstream-redis"
  }
}

# Application Load Balancer
resource "aws_lb" "rootstream" {
  name               = "rootstream-alb"
  internal           = false
  load_balancer_type = "application"
  security_groups    = [aws_security_group.alb.id]
  subnets            = aws_subnet.public[*].id
  
  enable_deletion_protection = false
  
  tags = {
    Name = "rootstream-alb"
  }
}

# S3 Bucket for storage
resource "aws_s3_bucket" "rootstream" {
  bucket = "rootstream-storage-${var.environment}"
  
  tags = {
    Name        = "rootstream-storage"
    Environment = var.environment
  }
}

resource "aws_s3_bucket_versioning" "rootstream" {
  bucket = aws_s3_bucket.rootstream.id
  
  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "rootstream" {
  bucket = aws_s3_bucket.rootstream.id
  
  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# CloudWatch Log Group
resource "aws_cloudwatch_log_group" "rootstream" {
  name              = "/aws/eks/rootstream"
  retention_in_days = var.log_retention_days
  
  tags = {
    Name = "rootstream-logs"
  }
}

# ECR Repository
resource "aws_ecr_repository" "rootstream" {
  name                 = "rootstream"
  image_tag_mutability = "MUTABLE"
  
  image_scanning_configuration {
    scan_on_push = true
  }
  
  tags = {
    Name = "rootstream-ecr"
  }
}
